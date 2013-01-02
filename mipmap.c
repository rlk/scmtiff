// SCMTIFF Copyright (C) 2012 Robert Kooima
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITH-
// OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "scm.h"
#include "scmdef.h"
#include "err.h"
#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

static void sum_pixel(float *p, const float *q0, const float *q1,
                                const float *q2, const float *q3, int c)
{
    switch (c)
    {
        case 4: p[3] = q0[3] + q1[3] + q2[3] + q3[3];
        case 3: p[2] = q0[2] + q1[2] + q2[2] + q3[2];
        case 2: p[1] = q0[1] + q1[1] + q2[1] + q3[1];
        case 1: p[0] = q0[0] + q1[0] + q2[0] + q3[0];
    }
}

static void max_pixel(float *p, const float *q0, const float *q1,
                                const float *q2, const float *q3, int c)
{
    switch (c)
    {
        case 4: p[3] = max(max(q0[3], q1[3]), max(q2[3], q3[3]));
        case 3: p[2] = max(max(q0[2], q1[2]), max(q2[2], q3[2]));
        case 2: p[1] = max(max(q0[1], q1[1]), max(q2[1], q3[1]));
        case 1: p[0] = max(max(q0[0], q1[1]), max(q2[0], q3[0]));
    }
}

static void avg_pixel(float *p, const float *q0, const float *q1,
                                const float *q2, const float *q3, int c)
{
    switch (c)
    {
        case 4: p[3] = (q0[3] + q1[3] + q2[3] + q3[3]) / 4.f;
        case 3: p[2] = (q0[2] + q1[2] + q2[2] + q3[2]) / 4.f;
        case 2: p[1] = (q0[1] + q1[1] + q2[1] + q3[1]) / 4.f;
        case 1: p[0] = (q0[0] + q1[0] + q2[0] + q3[0]) / 4.f;
    }
}

static void get_pixel(float *p, int ki, int kj,
                                int qi, int qj, int c, int n, int O, float *q)
{
    const int pi = qi / 2 + ki * n / 2;
    const int pj = qj / 2 + kj * n / 2;

    float *q0 = q + ((n + 2) * (qi + 1) + (qj + 1)) * c;
    float *q1 = q + ((n + 2) * (qi + 1) + (qj + 2)) * c;
    float *q2 = q + ((n + 2) * (qi + 2) + (qj + 1)) * c;
    float *q3 = q + ((n + 2) * (qi + 2) + (qj + 2)) * c;
    float *pp = p + ((n + 2) * (pi + 1) + (pj + 1)) * c;

    switch (O)
    {
        case 0: sum_pixel(pp, q0, q1, q2, q3, c); break;
        case 1: max_pixel(pp, q0, q1, q2, q3, c); break;
        case 2: avg_pixel(pp, q0, q1, q2, q3, c); break;
    }
}

// Box filter the c-channel, n-by-n image buffer q into quadrant (ki, kj) of
// the c-channel n-by-n image buffer p, downsampling 2-to-1.

static void box(float *p, int ki, int kj, int c, int n, int O, float *q)
{
    int qi;
    int qj;

    #pragma omp parallel for private(qj)
    for     (qi = 0; qi < n; qi += 2)
        for (qj = 0; qj < n; qj += 2)
            get_pixel(p, ki, kj, qi, qj, c, n, O, q);
}

// Scan SCM s seeking any page that is not present, but which has at least one
// child present. Fill such pages using down-sampled child data and append them.
// Return the number of pages added, so we can stop when there are none.

static long long process(scm *s, int O)
{
    long long t = 0;

    if (scm_scan_catalog(s))
    {
        // Determine the offset of the last page in the file.

        long long l = scm_get_length(s);
        long long b = 0;

        for (long long i = 0; i < l; ++i)
            if (b < scm_get_offset(s, i))
                b = scm_get_offset(s, i);

        // Allocate image buffers.

        float *p;
        float *q;

        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
        {
            for (long long x = 0; x < scm_get_index(s, 0); ++x)
            {
                // Calculate the page indices for all children of x.

                long long x0 = scm_page_child(x, 0);
                long long x1 = scm_page_child(x, 1);
                long long x2 = scm_page_child(x, 2);
                long long x3 = scm_page_child(x, 3);

                // Seek the catalog location of each page index.

                long long i1 = scm_search(s, x1);
                long long i2 = scm_search(s, x2);
                long long i3 = scm_search(s, x3);
                long long i0 = scm_search(s, x0);

                // Determine the file offset of each page.

                long long o0 = (i0 < 0) ? 0 : scm_get_offset(s, i0);
                long long o1 = (i1 < 0) ? 0 : scm_get_offset(s, i1);
                long long o2 = (i2 < 0) ? 0 : scm_get_offset(s, i2);
                long long o3 = (i3 < 0) ? 0 : scm_get_offset(s, i3);

                // Contribute any found offsets to the output.

                if (o0 || o1 || o2 || o3)
                {
                    const int o = scm_get_n(s) + 2;
                    const int n = scm_get_n(s);
                    const int c = scm_get_c(s);

                    memset(p, 0, (size_t) (o * o * c) * sizeof (float));

                    if (o0 && scm_read_page(s, o0, q)) box(p, 0, 0, c, n, O, q);
                    if (o1 && scm_read_page(s, o1, q)) box(p, 0, 1, c, n, O, q);
                    if (o2 && scm_read_page(s, o2, q)) box(p, 1, 0, c, n, O, q);
                    if (o3 && scm_read_page(s, o3, q)) box(p, 1, 1, c, n, O, q);

                    b = scm_append(s, b, x, p);
                    t++;
                }
            }
            free(q);
            free(p);
        }
    }
    return t;
}

//------------------------------------------------------------------------------

int mipmap(int argc, char **argv, const char *o, const char *m)
{
    int O = 2;

    if (m)
    {
        if      (strcmp(m, "sum") == 0) O = 0;
        else if (strcmp(m, "max") == 0) O = 1;
        else if (strcmp(m, "avg") == 0) O = 2;
    }

    if (argc > 0)
    {
        scm *s = NULL;

        if ((s = scm_ifile(argv[0])))
        {
            long long c;

            while ((c = process(s, O)))
                ;

            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
