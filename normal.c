// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"

//------------------------------------------------------------------------------

// Calculate the normal vector of the triangular face with vertices a, b, c.

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

// Generate normal vectors in buffer q using elevation values in buffer p.
// i and j give the pixel location in the c-channel n-by-n input image.
// u and v give the page location in the w-by-w subdivision of root face f.

static void sampnorm(const float *p, float *q,
                     int f, int i, int j, int n,
                     int c, int u, int v, int w, const float *r)
{
    const float dr = r[1] - r[0];

    float vn[3];
    float vs[3];
    float ve[3];
    float vw[3];
    float vc[3];

    scm_get_sample_center(f, n * u + i,     n * v + j,     n * w, vc);
    scm_get_sample_center(f, n * u + i - 1, n * v + j,     n * w, vn);
    scm_get_sample_center(f, n * u + i + 1, n * v + j,     n * w, vs);
    scm_get_sample_center(f, n * u + i,     n * v + j - 1, n * w, ve);
    scm_get_sample_center(f, n * u + i,     n * v + j + 1, n * w, vw);

    float rc = p[((n + 2) * (i + 1) + (j + 1)) * c] * dr + r[0];
    float rn = p[((n + 2) * (i + 0) + (j + 1)) * c] * dr + r[0];
    float rs = p[((n + 2) * (i + 2) + (j + 1)) * c] * dr + r[0];
    float re = p[((n + 2) * (i + 1) + (j + 0)) * c] * dr + r[0];
    float rw = p[((n + 2) * (i + 1) + (j + 2)) * c] * dr + r[0];

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

    float *o = q + 3 * ((n + 2) * (i + 1) + (j + 1));

    o[0] = na[0] + nb[0] + nc[0] + nd[0];
    o[1] = na[1] + nb[1] + nc[1] + nd[1];
    o[2] = na[2] + nb[2] + nc[2] + nd[2];

    normalize(o);

    o[0] = (o[0] + 1.f) / 2.f;
    o[1] = (o[1] + 1.f) / 2.f;
    o[2] = (o[2] + 1.f) / 2.f;
}

// Recursively traverse the tree at page x, reading input pages from SCM s and
// computing normal map output pages for SCM t.

static off_t divide(scm *s, off_t *o, float *p,
                    scm *t, off_t  b, float *q,
                    int f, int x, int d,
                    int u, int v, int w, const float *r)
{
    off_t a = b;

    if (d >= 0)
    {
        // Generate the normal map for page x.

        if (scm_read_page(s, o[x], p))
        {
            const int n = scm_get_n(s);
            const int c = scm_get_c(s);
            int i;
            int j;

            #pragma omp parallel for private(j)
            for     (i = 0; i < n; ++i)
                for (j = 0; j < n; ++j)
                    sampnorm(p, q, f, i, j, n, c, u, v, w, r);

            a = scm_append(t, a, x, q);
        }

        // Generate normal maps for the children of page x.

        int x0 = scm_get_page_child(x, 0);
        int x1 = scm_get_page_child(x, 1);
        int x2 = scm_get_page_child(x, 2);
        int x3 = scm_get_page_child(x, 3);

        int u0 = u * 2, u1 = u0 + 1;
        int v0 = v * 2, v1 = v0 + 1;

        if (x0) a = divide(s, o, p, t, a, q, f, x0, d - 1, u0, v0, 2 * w, r);
        if (x1) a = divide(s, o, p, t, a, q, f, x1, d - 1, u0, v1, 2 * w, r);
        if (x2) a = divide(s, o, p, t, a, q, f, x2, d - 1, u1, v0, 2 * w, r);
        if (x3) a = divide(s, o, p, t, a, q, f, x3, d - 1, u1, v1, 2 * w, r);
    }
    return a;
}

// Query the offsets of all pages in SCM s. Allocate I/O scratch buffers and
// initiate the recursive traversal of all pages.

static void process(scm *s, scm *t, const float *r)
{
    int    d;
    off_t *o;
    float *p;
    float *q;

    if ((d = scm_mapping(s, &o)) >= 0)
    {
        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(t)))
        {
            off_t b = 0;

            memset(q, 0, 3 * (scm_get_n(t) + 2) *
                             (scm_get_n(t) + 2) * sizeof (float));

            if (o[0]) b = divide(s, o, p, t, b, q, 0, 0, d, 0, 0, 1, r);
            if (o[1]) b = divide(s, o, p, t, b, q, 1, 1, d, 0, 0, 1, r);
            if (o[2]) b = divide(s, o, p, t, b, q, 2, 2, d, 0, 0, 1, r);
            if (o[3]) b = divide(s, o, p, t, b, q, 3, 3, d, 0, 0, 1, r);
            if (o[4]) b = divide(s, o, p, t, b, q, 4, 4, d, 0, 0, 1, r);
            if (o[5]) b = divide(s, o, p, t, b, q, 5, 5, d, 0, 0, 1, r);

            free(q);
            free(p);
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
            char *T = scm_get_description(s);
            int   n = scm_get_n(s);

            if ((t = scm_ofile(out, n, 3, 8, 0, T)))
            {
                process(s, t, R);
                scm_close(t);
            }
            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
