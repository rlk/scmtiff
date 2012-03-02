// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"

//------------------------------------------------------------------------------

static void facenorm(const float *a, const float *b, const float *c, float *n)
{
    float u[3];
    float v[3];

    u[0] = b[0] - a[0];
    u[1] = b[1] - a[1];
    u[2] = b[2] - a[2];

    v[0] = c[0] - a[0];
    v[1] = c[1] - a[1];
    v[2] = c[2] - a[2];

    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];

    normalize(n);
}

static void sampnorm(const float *u,
                     const float *p, int n, int c,
                           float *q, int i, int j, float r0, float r1)
{
    float vn[3];
    float vs[3];
    float ve[3];
    float vw[3];
    float vc[3];

    scm_get_samp_vector(u, i,     j,     n, vc);
    scm_get_samp_vector(u, i - 1, j,     n, vn);
    scm_get_samp_vector(u, i + 1, j,     n, vs);
    scm_get_samp_vector(u, i,     j - 1, n, ve);
    scm_get_samp_vector(u, i,     j + 1, n, vw);

    float rc = p[((n + 2) * (i + 1) + (j + 1)) * c] * (r1 - r0) + r0;
    float rn = p[((n + 2) * (i + 0) + (j + 1)) * c] * (r1 - r0) + r0;
    float rs = p[((n + 2) * (i + 2) + (j + 1)) * c] * (r1 - r0) + r0;
    float re = p[((n + 2) * (i + 1) + (j + 0)) * c] * (r1 - r0) + r0;
    float rw = p[((n + 2) * (i + 1) + (j + 2)) * c] * (r1 - r0) + r0;

    vn[0] *= rn; vn[1] *= rn; vn[2] *= rn;
    vs[0] *= rs; vs[1] *= rs; vs[2] *= rs;
    ve[0] *= re; ve[1] *= re; ve[2] *= re;
    vw[0] *= rw; vw[1] *= rw; vw[2] *= rw;
    vc[0] *= rc; vc[1] *= rc; vc[2] *= rc;

    float na[3];
    float nb[3];
    float nc[3];
    float nd[3];

    facenorm(vc, vn, ve, na);
    facenorm(vc, ve, vs, nb);
    facenorm(vc, vs, vw, nc);
    facenorm(vc, vw, vn, nd);

    float *v = q + 3 * ((n + 2) * (i + 1) + (j + 1));

    v[0] = na[0] + nb[0] + nc[0] + nd[0];
    v[1] = na[1] + nb[1] + nc[1] + nd[1];
    v[2] = na[2] + nb[2] + nc[2] + nd[2];

    normalize(v);

    v[0] = (v[0] + 1.f) / 2.f;
    v[1] = (v[1] + 1.f) / 2.f;
    v[2] = (v[2] + 1.f) / 2.f;
}

static void process(scm *s, scm *t, float r0, float r1)
{
    off_t *o;
    int    d;

    if ((d = scm_mapping(s, &o)) >= 0)
    {
        const int m = scm_get_page_count(d);
        float *v;

        if ((v = (float *) malloc(12 * m * sizeof (float))))
        {
            float *p;
            float *q;

            scm_get_page_corners(d, v);

            if ((p = scm_alloc_buffer(s)) &&
                (q = scm_alloc_buffer(t)))
            {
                off_t b = 0;

                for (int x = 0; x < m; ++x)

                    if (o[x] && scm_read_page(s, o[x], p))
                    {
                        const int n = scm_get_n(s);
                        const int c = scm_get_c(s);
                        int i;
                        int j;

                        memset(q, 0, (n + 2) * (n + 2) * 3 * sizeof (float));

                        #pragma omp parallel for private(j)
                        for     (i = 0; i < n; ++i)
                            for (j = 0; j < n; ++j)
                                sampnorm(v + x * 12, p, n, c, q, i, j, r0, r1);

                        b = scm_append(t, b, x, q);
                    }

                free(q);
                free(p);
            }
            free(v);
        }
        free(o);
    }
}

//------------------------------------------------------------------------------

int normal(int argc, char **argv, const char *o, const float *R)
{
    if (argc > 0)
    {
        const char *out = o ? o : "out.tif";

        scm *s;
        scm *t;

        if ((s = scm_ifile(argv[0])))
        {
            int   n = scm_get_n(s);
            char *T = scm_get_description(s);

            if ((t = scm_ofile(out, n, 3, 8, 0, T)))
            {
                process(s, t, R[0], R[1]);
                scm_close(t);
            }
            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
