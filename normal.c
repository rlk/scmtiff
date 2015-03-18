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
#include <string.h>

#include "scm.h"
#include "scmdef.h"
#include "err.h"
#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

// Calculate the normal vector of the triangular face with vertices a, b, c.

static void facenorm(const double *a, const double *b, const double *c, double *n)
{
    double u[3];
    double v[3];

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

static void sampnorm(int f, int i, int j, int n, int c,
                     long u, long v, long w, const float *r, const float *p, float *q)
{
    const float dr = r[1] - r[0];

    double nn[3] = { 0, 0, 0 };
    double nt[3], rt;

    double vc[3];
    double vn[3];
    double vs[3];
    double vw[3];
    double ve[3];

    scm_get_sample_center(f, (n - 1) * u + i, (n - 1) * v + j, (n - 1) * w, vc);
    rt = p[(n * i + j) * c] * dr + r[0];
    vc[0] *= rt;
    vc[1] *= rt;
    vc[2] *= rt;

    if (i > 0)
    {
        scm_get_sample_center(f, (n - 1) * u + i - 1, (n - 1) * v + j, (n - 1) * w, vn);
        rt = p[(n * (i - 1) + j) * c] * dr + r[0];
        vn[0] *= rt;
        vn[1] *= rt;
        vn[2] *= rt;
    }
    if (i < n - 1)
    {
        scm_get_sample_center(f, (n - 1) * u + i + 1, (n - 1) * v + j, (n - 1) * w, vs);
        rt = p[(n * (i + 1) + j) * c] * dr + r[0];
        vs[0] *= rt;
        vs[1] *= rt;
        vs[2] *= rt;
    }
    if (j > 0)
    {
        scm_get_sample_center(f, (n - 1) * u + i, (n - 1) * v + j - 1, (n - 1) * w, vw);
        rt = p[(n * i + (j - 1)) * c] * dr + r[0];
        vw[0] *= rt;
        vw[1] *= rt;
        vw[2] *= rt;
    }
    if (j < n - 1)
    {
        scm_get_sample_center(f, (n - 1) * u + i, (n - 1) * v + j + 1, (n - 1) * w, ve);
        rt = p[(n * i + (j + 1)) * c] * dr + r[0];
        ve[0] *= rt;
        ve[1] *= rt;
        ve[2] *= rt;
    }

    if (i > 0 && j > 0)
    {
        facenorm(vc, vn, vw, nt);
        nn[0] += nt[0];
        nn[1] += nt[1];
        nn[2] += nt[2];
    }
    if (i > 0 && j < n - 1)
    {
        facenorm(vc, ve, vn, nt);
        nn[0] += nt[0];
        nn[1] += nt[1];
        nn[2] += nt[2];
    }
    if (i < n - 1 && j > 0)
    {
        facenorm(vc, vw, vs, nt);
        nn[0] += nt[0];
        nn[1] += nt[1];
        nn[2] += nt[2];
    }
    if (i < n - 1 && j < n - 1)
    {
        facenorm(vc, vs, ve, nt);
        nn[0] += nt[0];
        nn[1] += nt[1];
        nn[2] += nt[2];
    }

    normalize(nn);

    float *o = q + 3 * (n * i + j);

    o[0] = ((float) nn[0] + 1.f) / 2.f;
    o[1] = ((float) nn[1] + 1.f) / 2.f;
    o[2] = ((float) nn[2] + 1.f) / 2.f;
}

// Recursively traverse the tree at page x, reading input pages from SCM s and
// computing normal map output pages for SCM t.

static long long divide(scm *s, long long x,
                        scm *t, long long b,
                        long u, long v, long w,
                        const float *r, float *p, float *q)
{
    long long i;

    if ((i = scm_search(s, x)) >= 0)
    {
        long long o = scm_get_offset(s, i);

        // Generate the normal map for page x.

        if (scm_read_page(s, o, p))
        {
            const int f = scm_page_root(x);
            const int n = scm_get_n(s);
            const int c = scm_get_c(s);
            int i;
            int j;

            #pragma omp parallel for private(j)
            for     (i = 0; i < n; ++i)
                for (j = 0; j < n; ++j)
                    sampnorm(f, i, j, n, c, u, v, w, r, p, q);

            b = scm_append(t, b, x, q);
        }

        // Generate normal maps for the children of page x.

        long long x0 = scm_page_child(x, 0);
        long long x1 = scm_page_child(x, 1);
        long long x2 = scm_page_child(x, 2);
        long long x3 = scm_page_child(x, 3);

        long u0 = u * 2, u1 = u0 + 1;
        long v0 = v * 2, v1 = v0 + 1;

        if (x0) b = divide(s, x0, t, b, u0, v0, 2 * w, r, p, q);
        if (x1) b = divide(s, x1, t, b, u0, v1, 2 * w, r, p, q);
        if (x2) b = divide(s, x2, t, b, u1, v0, 2 * w, r, p, q);
        if (x3) b = divide(s, x3, t, b, u1, v1, 2 * w, r, p, q);
    }
    return b;
}

// Query the offsets of all pages in SCM s. Allocate I/O scratch buffers and
// initiate the recursive traversal of all pages.

static void process(scm *s, scm *t, const float *r)
{
    float *p;
    float *q;

    if (scm_scan_catalog(s))
    {
        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(t)))
        {
            long long b = 0;

            memset(q, 0, 3 * (size_t) scm_get_n(t) *
                             (size_t) scm_get_n(t) * sizeof (float));

            b = divide(s, 0, t, b, 0, 0, 1, r, p, q);
            b = divide(s, 1, t, b, 0, 0, 1, r, p, q);
            b = divide(s, 2, t, b, 0, 0, 1, r, p, q);
            b = divide(s, 3, t, b, 0, 0, 1, r, p, q);
            b = divide(s, 4, t, b, 0, 0, 1, r, p, q);
            b = divide(s, 5, t, b, 0, 0, 1, r, p, q);

            free(q);
            free(p);
        }
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
            if ((t = scm_ofile(out, scm_get_n(s), 3, 8, 0)))
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
