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
    long long  b = 0;
    long long *a;
    int        d;
    long long  l;

    l = scm_read_catalog(s, &a);
    scm_sort_catalog(a, l);

    for (long long i = 0; i < l; ++i)
        printf("%lld %lld\n", a[2 * i + 0], a[2 * i + 1]);


    if ((d = scm_mapping(s, &a)) >= 0)
    {
        const long long m = scm_get_page_count(d - 1);
        const int       o = scm_get_n(s) + 2;
        const int       n = scm_get_n(s);
        const int       c = scm_get_c(s);

        float *p;
        float *q;

        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
        {
            for (long long x = 0; x < m; ++x)
            {
                long long x0 = scm_get_page_child(x, 0);
                long long x1 = scm_get_page_child(x, 1);
                long long x2 = scm_get_page_child(x, 2);
                long long x3 = scm_get_page_child(x, 3);

                if (a[x] == 0 && (a[x0] || a[x1] || a[x2] || a[x3]))
                {
                    memset(p, 0, (size_t) (o * o * c) * sizeof (float));

                    if (a[x0] && scm_read_page(s, a[x0], q)) box(p, 0, 0, c, n, q);
                    if (a[x1] && scm_read_page(s, a[x1], q)) box(p, 0, 1, c, n, q);
                    if (a[x2] && scm_read_page(s, a[x2], q)) box(p, 1, 0, c, n, q);
                    if (a[x3] && scm_read_page(s, a[x3], q)) box(p, 1, 1, c, n, q);

                    b = scm_append(t, b, x, p);
                }
            }
            free(q);
            free(p);
        }
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
