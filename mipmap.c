// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"

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

            float *qq = q + ((qi + 1) * (n + 2) + (qj + 1)) * c;
            float *pp = p + ((pi + 1) * (n + 2) + (pj + 1)) * c;

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

static off_t sample(scm *s, scm *t)
{
    off_t  b = 0;
    off_t *m;
    int    d;

    if ((d = scm_mapping(s, &m)) >= 0)
    {
        const int M = scm_get_page_count(d - 1);
        const int N = scm_get_n(s) + 2;
        const int n = scm_get_n(s);
        const int c = scm_get_c(s);

        float *p;
        float *q;

        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
        {
            for (int x = 0; x < M; ++x)
            {
                int i = scm_get_page_child(x, 0);
                int j = scm_get_page_child(x, 1);
                int k = scm_get_page_child(x, 2);
                int l = scm_get_page_child(x, 3);

                if (m[x] == 0 && (m[i] || m[j] || m[k] || m[l]))
                {
                    memset(p, 0, N * N * c * sizeof (float));

                    if (m[i] && scm_read_page(s, m[i], q)) box(p, 0, 0, c, n, q);
                    if (m[j] && scm_read_page(s, m[j], q)) box(p, 0, 1, c, n, q);
                    if (m[k] && scm_read_page(s, m[k], q)) box(p, 1, 0, c, n, q);
                    if (m[l] && scm_read_page(s, m[l], q)) box(p, 1, 1, c, n, q);

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

static void append(scm *s, scm *t, off_t b)
{
    float *p;

    if ((p = scm_alloc_buffer(s)))
    {
        off_t o;
        off_t n;

        int x;

        for (o = scm_rewind(s); (x = scm_read_node(s, o, &n, 0)) >= 0; o = n)
            b = scm_repeat(t, b, s, o);
    }
}

// Mipmap SCM s to the fullest extent possible. To do so,  When finished,
// append the contents of SCM s to SCM t. Return failure to indicate that no
// processing was performed, implying that mipmapping of SCM s is complete.

static int process(scm *s, scm *t)
{
    off_t b;

    if ((b = sample(s, t)))
    {
        append(s, t, b);
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

//------------------------------------------------------------------------------

int mipmap(int argc, char **argv)
{
    const char *out = "out.tif";
    const char *in  = "in.tif";

    scm *s = NULL;
    scm *t = NULL;

    int r = EXIT_FAILURE;

    for (int i = 1; i < argc; ++i)
        if      (strcmp(argv[i],   "-o") == 0) out = argv[++i];
        else if (extcmp(argv[i], ".tif") == 0) in  = argv[  i];

    if ((s = scm_ifile(in)))
    {
        char *str = scm_get_description(s);

        int n = scm_get_n(s);
        int c = scm_get_c(s);
        int b = scm_get_b(s);
        int g = scm_get_g(s);

        if ((t = scm_ofile(out, n, c, b, g, str)))
        {
            r = process(s, t);
            scm_relink(t);
            scm_close(t);
        }
        scm_close(s);
    }
    return r;
}

//------------------------------------------------------------------------------
