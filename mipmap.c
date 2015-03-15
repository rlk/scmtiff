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

static void copy(float *p, int ki, int kj, int c, int n, float *q)
{
    int i;
    int j;

    #pragma omp parallel for private(j)
    for     (i = 0; i < n; i += 2)
        for (j = 0; j < n; j += 2)
        {
            int pi = ki ? n - 1 - i / 2 : i / 2;
            int pj = kj ? n - 1 - j / 2 : j / 2;
            int qi = ki ? n - 1 - i     : i;
            int qj = kj ? n - 1 - j     : j;

            float *pp = p + (n * pi + pj) * c;
            float *qq = q + (n * qi + qj) * c;

            switch (c)
            {
                case 4: pp[3] = qq[3];
                case 3: pp[2] = qq[2];
                case 2: pp[1] = qq[1];
                case 1: pp[0] = qq[0];
            }
        }
}

//------------------------------------------------------------------------------

// Scan SCM s seeking any page that is not present, but which has at least one
// child present. Fill such pages using down-sampled child data and append them.
// Return the number of pages added, so we can stop when there are none.

static long long process(scm *s, int O, int A)
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
                    const int n = scm_get_n(s);
                    const int c = scm_get_c(s);

                    memset(p, 0, (size_t) (n * n * c) * sizeof (float));

                    if (o0 && scm_read_page(s, o0, q)) copy(p, 0, 0, c, n, q);
                    if (o1 && scm_read_page(s, o1, q)) copy(p, 0, 1, c, n, q);
                    if (o2 && scm_read_page(s, o2, q)) copy(p, 1, 0, c, n, q);
                    if (o3 && scm_read_page(s, o3, q)) copy(p, 1, 1, c, n, q);

                    if (A) grow(p, q, c, n);

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

int mipmap(int argc, char **argv, const char *o, const char *m, int A)
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

            while ((c = process(s, O, A)))
                ;

            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
