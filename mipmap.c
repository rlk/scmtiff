// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

// Box filter the c-channel, n-by-n image buffer q into quadrant (ki, kj) of
// the c-channel n-by-n image buffer p, downsampling 2-to-1.

static void box(float *p, int ki, int kj, int c, int n, float *q)
{
    for     (int qi = 0; qi < n; ++qi)
        for (int qj = 0; qj < n; ++qj)
        {
            const int pi = qi / 2 + ki * n / 2;
            const int pj = qj / 2 + kj * n / 2;

            float *qq = q + ((n + 2) * (qi + 1) + (qj + 1)) * c;
            float *pp = p + ((n + 2) * (pi + 1) + (pj + 1)) * c;

            switch (c)
            {
                case 4: pp[3] += qq[3] / 4.f;
                case 3: pp[2] += qq[2] / 4.f;
                case 2: pp[1] += qq[1] / 4.f;
                case 1: pp[0] += qq[0] / 4.f;
            }
        }
}

// scan SCM s seeking any page that is not present, but which has at least one
// child present. Fill such pages using down-sampled child data and append them
// to SCM t.

static long long sample(scm *s, scm *t)
{
    long long b = 0;
    long long l;
    scm_pair *a;

    if ((l = scm_read_catalog(s, &a)))
    {
        scm_sort_catalog(a, l);

        float *p;
        float *q;

        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
        {
            for (long long x = 0; x < a[0].x; ++x)
            {
                // Calculate the page indices for all children of x.

                long long x0 = scm_get_page_child(x, 0);
                long long x1 = scm_get_page_child(x, 1);
                long long x2 = scm_get_page_child(x, 2);
                long long x3 = scm_get_page_child(x, 3);

                // Seek the catalog location of each page index.

                long long i0 = scm_seek_catalog(a,      0,  l, x0);
                long long i1 = scm_seek_catalog(a, i0 + 1, l, x1);
                long long i2 = scm_seek_catalog(a, i1 + 1, l, x2);
                long long i3 = scm_seek_catalog(a, i2 + 1, l, x3);

                // Look up the file offset for each located page.

                long long o0 = (i0 < 0) ? 0 : a[i0].o;
                long long o1 = (i1 < 0) ? 0 : a[i1].o;
                long long o2 = (i2 < 0) ? 0 : a[i2].o;
                long long o3 = (i3 < 0) ? 0 : a[i3].o;

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

                    b = scm_append(t, b, x, p);
                }
            }
            free(q);
            free(p);
        }
        free(a);
    }
    return b;
}

// Append the full contents of SCM s to the current contents of SCM t.

static void append(scm *s, scm *t, long long b)
{
    float *p;

    if ((p = scm_alloc_buffer(s)))
    {
        long long o;
        long long n;
        long long x;

        for (o = scm_rewind(s); (x = scm_read_node(s, o, &n, 0)) >= 0; o = n)
            b = scm_repeat(t, b, s, o);
    }
}

// Mipmap SCM s to the fullest extent possible. To do so,  When finished,
// append the contents of SCM s to SCM t. Return failure to indicate that no
// processing was performed, implying that mipmapping of SCM s is complete.

static bool process(scm *s, scm *t)
{
    long long b;

    if ((b = sample(s, t)))
    {
        append(s, t, b);
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------

int mipmap(int argc, char **argv, const char *o)
{
    if (argc > 0)
    {
        const char *out = o ? o : "out.tif";

        scm *s = NULL;
        scm *t = NULL;

        if ((s = scm_ifile(argv[0])))
        {
            int   n = scm_get_n(s);
            int   c = scm_get_c(s);
            int   b = scm_get_b(s);
            int   g = scm_get_g(s);
            char *T = scm_get_description(s);

            if ((t = scm_ofile(out, n, c, b, g, T)))
            {
                process(s, t);
                scm_close(t);
            }
            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
