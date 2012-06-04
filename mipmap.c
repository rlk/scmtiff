// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

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

// Box filter the c-channel, n-by-n image buffer q into quadrant (ki, kj) of
// the c-channel n-by-n image buffer p, downsampling 2-to-1. Don't factor in
// zeros.

static void box(float *p, int ki, int kj, int c, int n, float *q)
{
    int qi;
    int qj;

    #pragma omp parallel for private(qj)
    for     (qi = 0; qi < n; qi += 2)
        for (qj = 0; qj < n; qj += 2)
        {
            const int pi = qi / 2 + ki * n / 2;
            const int pj = qj / 2 + kj * n / 2;

            float *q0 = q + ((n + 2) * (qi + 1) + (qj + 1)) * c;
            float *q1 = q + ((n + 2) * (qi + 1) + (qj + 2)) * c;
            float *q2 = q + ((n + 2) * (qi + 2) + (qj + 1)) * c;
            float *q3 = q + ((n + 2) * (qi + 2) + (qj + 2)) * c;
            float *pp = p + ((n + 2) * (pi + 1) + (pj + 1)) * c;

            int k;

            for (k = 0; k < c; ++k)
            {
                int s = 0;

                if (q0[k] > 0) s++;
                if (q1[k] > 0) s++;
                if (q2[k] > 0) s++;
                if (q3[k] > 0) s++;

                pp[k] = s ? (q0[k] + q1[k] + q2[k] + q3[k]) / s : 0;
            }
        }
}

// Scan SCM s seeking any page that is not present, but which has at least one
// child present. Fill such pages using down-sampled child data and append them.
// Return the number of pages added, so we can stop when there are none.

static long long sample(scm *s)
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

                    if (o0 && scm_read_page(s, o0, q)) box(p, 0, 0, c, n, q);
                    if (o1 && scm_read_page(s, o1, q)) box(p, 0, 1, c, n, q);
                    if (o2 && scm_read_page(s, o2, q)) box(p, 1, 0, c, n, q);
                    if (o3 && scm_read_page(s, o3, q)) box(p, 1, 1, c, n, q);

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

int mipmap(int argc, char **argv, const char *o)
{
    if (argc > 0)
    {
        scm *s = NULL;

        if ((s = scm_ifile(argv[0])))
        {
            long long c;

            while ((c = sample(s)))
                ;

            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
