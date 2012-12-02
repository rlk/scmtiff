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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "scm.h"
#include "scmdef.h"
#include "img.h"
#include "err.h"
#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

// Given the four corner vectors of a sample, compute the five internal vectors
// of a quincunx filtering of that sample.

static void quincunx(double *q, const double *v)
{
    mid4(q + 12, v +  0, v +  3, v +  6, v +  9);
    mid2(q +  9, q + 12, v +  9);
    mid2(q +  6, q + 12, v +  6);
    mid2(q +  3, q + 12, v +  3);
    mid2(q +  0, q + 12, v +  0);
}

// Compute the corner vectors of the pixel at row i column j of the n-by-n page
// found at row u column v of the w-by-w page array on face f. (yow!) Sample
// that pixel by projection into image p using a quincunx filtering pattern.
// This do-it-all function forms the kernel of the OpenMP parallelization.

static int corner(img *p, int  f, int  i, int  j, int n,
                          long u, long v, long w, float *q)
{
    double c[12];
    double C[15];
    int    D = 0;

    scm_get_sample_corners(f, n * u + i, n * v + j, n * w, c);
    quincunx(C, c);

    for (int l = 4; l >= 0; --l)
    {
        float t[4];
        int d;

        if ((d = img_sample(p, C + l * 3, t)))
        {
            switch (p->c)
            {
                case 4: q[3] += t[3];
                case 3: q[2] += t[2];
                case 2: q[1] += t[1];
                case 1: q[0] += t[0];
            }
            D += d;
        }
    }
    if (D)
    {
        switch (p->c)
        {
            case 4: q[3] /= D;
            case 3: q[2] /= D;
            case 2: q[1] /= D;
            case 1: q[0] /= D;
        }
        return 1;
    }
    return 0;
}

// Determine whether the image intersects with the page at row u column v of the
// w-by-w page array on face f. This function is inefficient and only works
// under limited circumstances.

static bool overlap(img *p, int f, long u, long v, long w)
{
    const int n = 128;

    for     (int i = 0; i <= n; ++i)
        for (int j = 0; j <= n; ++j)
        {
            double c[3];

            scm_get_sample_center(f, n * u + i, n * v + j, n * w, c);

            if (img_locate(p, c))
                return true;
        }

    return false;
}

// Consider page x of SCM s. Determine whether it contains any of image p.
// If so, sample it or recursively subdivide it as needed.

static long long divide(scm *s, long long b, int  d, long long x,
                                long u, long v, long w, img *p, float *q)
{
    long long a = b;

    if (overlap(p, scm_page_root(x), u, v, w))
    {
        if (d == 0)
        {
            const int f = scm_page_root(x);
            const int o = scm_get_n(s) + 2;
            const int c = scm_get_c(s);
            const int n = scm_get_n(s);

            int h = 0;
            int i;
            int j;

            memset(q, 0, (size_t) (o * o * c) * sizeof (float));

            #pragma omp parallel for private(j) reduction(+:h)
            for     (i = 0; i < n; ++i)
                for (j = 0; j < n; ++j)
                    h += corner(p, f, i, j, n, u, v, w,
                                q + c * (o * (i + 1) + (j + 1)));

            if (h) a = scm_append(s, a, x, q);
        }
        else
        {
            long long x0 = scm_page_child(x, 0);
            long long x1 = scm_page_child(x, 1);
            long long x2 = scm_page_child(x, 2);
            long long x3 = scm_page_child(x, 3);

            a = divide(s, a, d - 1, x0, u * 2,     v * 2,     w * 2, p, q);
            a = divide(s, a, d - 1, x1, u * 2,     v * 2 + 1, w * 2, p, q);
            a = divide(s, a, d - 1, x2, u * 2 + 1, v * 2,     w * 2, p, q);
            a = divide(s, a, d - 1, x3, u * 2 + 1, v * 2 + 1, w * 2, p, q);
        }
    }
    return a;
}

// Convert image p to SCM s with depth d. Allocate working buffers and perform a
// depth-first traversal of the page tree.

static int process(scm *s, int d, img *p)
{
    const size_t o = (size_t) scm_get_n(s) + 2;
    const size_t c = (size_t) scm_get_c(s);

    float *q;

    if ((q = (float *) calloc(o * o * c, sizeof (float))))
    {
        long long b = 0;

        b = divide(s, b, d, 0, 0, 0, 1, p, q);
        b = divide(s, b, d, 1, 0, 0, 1, p, q);
        b = divide(s, b, d, 2, 0, 0, 1, p, q);
        b = divide(s, b, d, 3, 0, 0, 1, p, q);
        b = divide(s, b, d, 4, 0, 0, 1, p, q);
        b = divide(s, b, d, 5, 0, 0, 1, p, q);

        free(q);
    }
    return 0;
}

//------------------------------------------------------------------------------

int convert(int argc, char **argv, const char *o,
                                           int n,
                                           int d,
                                           int b,
                                           int g,
                                           int x,
                                 const double *L,
                                 const double *P,
                                 const float  *N)
{
    img  *p = NULL;
    scm  *s = NULL;
    char *e = NULL;

    char out[256];

    // Iterate over all input file arguments.

    for (int i = 0; i < argc; i++)
    {
        const char *in = argv[i];

        // Generate the output file name.

        if (o) strcpy(out, o);

        else if ((e = strrchr(in, '.')))
        {
            memset (out, 0, 256);
            strncpy(out, in, e - in);
            strcat (out, ".tif");
        }
        else strcpy(out, "out.tif");

        // Load the input file.

        if      (extcmp(in, ".jpg") == 0) p = jpg_load(in);
        else if (extcmp(in, ".png") == 0) p = png_load(in);
        else if (extcmp(in, ".tif") == 0) p = tif_load(in);
        else if (extcmp(in, ".img") == 0) p = pds_load(in);
        else if (extcmp(in, ".lbl") == 0) p = pds_load(in);

        if (p)
        {
            // Allow the channel format overrides.

            if (b == -1) b = p->b;
            if (g == -1) g = p->g;

            if (x >= 0)
            {
                p->project = img_scube;
                p->x       = x;
            }

            // Set the blending parameters.

            p->latc = P[0] * M_PI / 180.0;
            p->lat0 = P[1] * M_PI / 180.0;
            p->lat1 = P[2] * M_PI / 180.0;

            p->lonc = L[0] * M_PI / 180.0;
            p->lon0 = L[1] * M_PI / 180.0;
            p->lon1 = L[2] * M_PI / 180.0;

            // Set the normalization parameters.

            if (N[0] || N[1])
            {
                p->norm0 = N[0];
                p->norm1 = N[1];
            }
            else if (b == 8)
            {
                if (g) { p->norm0 = 0.0f; p->norm1 =   127.0f; }
                else   { p->norm0 = 0.0f; p->norm1 =   255.0f; }
            }
            else if (b == 16)
            {
                if (g) { p->norm0 = 0.0f; p->norm1 = 32767.0f; }
                else   { p->norm0 = 0.0f; p->norm1 = 65535.0f; }
            }
            else
            {
                p->norm0 = 0.0f;
                p->norm1 = 1.0f;
            }

            // Process the output.

            if ((s = scm_ofile(out, n, p->c, b, g)))
            {
                process(s, d, p);
                scm_close(s);
            }
            img_close(p);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
