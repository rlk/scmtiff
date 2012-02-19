// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "scm.h"
#include "img.h"
#include "util.h"

//------------------------------------------------------------------------------

static void normalize(double *v)
{
    double k = 1.0 / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= k;
    v[1] *= k;
    v[2] *= k;
}

static void sample_vectors(double *w, const double *v)
{
    mid4(w + 12, v +  0, v +  3, v +  6, v +  9);
    mid2(w +  9, w + 12, v +  9);
    mid2(w +  6, w + 12, v +  6);
    mid2(w +  3, w + 12, v +  3);
    mid2(w +  0, w + 12, v +  0);
}

static void corner_vectors(double *v, const double *u, int r, int c, int n)
{
    const double r0 = (double) (r + 0) / n;
    const double r1 = (double) (r + 1) / n;
    const double c0 = (double) (c + 0) / n;
    const double c1 = (double) (c + 1) / n;

    slerp2(v + 0, u + 0, u + 3, u + 6, u + 9, c0, r0);
    slerp2(v + 3, u + 0, u + 3, u + 6, u + 9, c1, r0);
    slerp2(v + 6, u + 0, u + 3, u + 6, u + 9, c0, r1);
    slerp2(v + 9, u + 0, u + 3, u + 6, u + 9, c1, r1);

    normalize(v + 0);
    normalize(v + 3);
    normalize(v + 6);
    normalize(v + 9);
}

//------------------------------------------------------------------------------

// Compute the corner vectors of the pixel at row i column j of the n-by-n page
// with corners c. Sample that pixel by projection into image p using a quincunx
// filtering patterns. This do-it-all function forms the kernel of the OpenMP
// parallelization.

static int sample(img *p, int i, int j, int n, const double *c, double *x)
{
    double v[12];
    double w[15];
    double A = 0;

    corner_vectors(v, c, i, j, n);
    sample_vectors(w, v);

    for (int l = 4; l >= 0; --l)
    {
        double t[4];
        double a;

        if ((a = img_sample(p, w + l * 3, t)))
        {
            switch (p->c)
            {
                case 4: x[3] += t[3] / 5.0;
                case 3: x[2] += t[2] / 5.0;
                case 2: x[1] += t[1] / 5.0;
                case 1: x[0] += t[0] / 5.0;
            }

            A += a;
        }
    }
    return (A > 0.0);
}

int process(scm *s, img *p, int d)
{
    const int M = scm_get_page_count(d);
    const int n = scm_get_n(s);
    const int N = scm_get_n(s) + 2;
    const int C = scm_get_c(s);

    double *vec;
    double *dat;

    if ((vec = (double *) calloc(M * 12,    sizeof (double))) &&
        (dat = (double *) calloc(N * N * C, sizeof (double))))
    {
        const int x0 = d ? scm_get_page_count(d - 1) : 0;
        const int x1 = d ? scm_get_page_count(d)     : 6;

        off_t b = 0;
        int   x;

        scm_get_page_corners(d, vec);

        for (x = x0; x < x1; ++x)
        {
            int k = 0;
            int r;
            int c;

            memset(dat, 0, N * N * C * sizeof (double));

            #pragma omp parallel for private(c) reduction(+:k)

            for     (r = 0; r < n; ++r)
                for (c = 0; c < n; ++c)
                    k += sample(p, r, c, N, vec + x * 12,
                                            dat + C * ((r + 1) * N + (c + 1)));

            if (k) b = scm_append(s, b, 0, 0, x, dat);
        }

        free(dat);
        free(vec);
    }

    return 0;
}

int convert(int argc, char **argv)
{
    const char *t = "Copyright (c) 2011 Robert Kooima";
    const char *o = "out.tif";
    int         n = 512;
    int         d = 0;
    int         b = 0;
    int         g = 0;

    double  lat0 = 0.0;
    double  lat1 = 0.0;
    double  lon0 = 0.0;
    double  lon1 = 0.0;
    double dlat0 = 0.0;
    double dlat1 = 0.0;
    double dlon0 = 0.0;
    double dlon1 = 0.0;

    img *p = NULL;
    scm *s = NULL;

    // Parse command line options.

    int i;

    for (i = 1; i < argc; ++i)
        if      (strcmp(argv[i], "-o")     == 0)  o    =      argv[++i];
        else if (strcmp(argv[i], "-t")     == 0)  t    =      argv[++i];
        else if (strcmp(argv[i], "-n")     == 0)  n    = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d")     == 0)  d    = atoi(argv[++i]);
        else if (strcmp(argv[i], "-b")     == 0)  b    = atoi(argv[++i]);
        else if (strcmp(argv[i], "-g")     == 0)  g    = atoi(argv[++i]);
        else if (strcmp(argv[i], "-lat0")  == 0)  lat0 = atof(argv[++i]);
        else if (strcmp(argv[i], "-lat1")  == 0)  lat1 = atof(argv[++i]);
        else if (strcmp(argv[i], "-lon0")  == 0)  lon0 = atof(argv[++i]);
        else if (strcmp(argv[i], "-lon1")  == 0)  lon1 = atof(argv[++i]);
        else if (strcmp(argv[i], "-dlat0") == 0) dlat0 = atof(argv[++i]);
        else if (strcmp(argv[i], "-dlat1") == 0) dlat1 = atof(argv[++i]);
        else if (strcmp(argv[i], "-dlon0") == 0) dlon0 = atof(argv[++i]);
        else if (strcmp(argv[i], "-dlon1") == 0) dlon1 = atof(argv[++i]);

    // Assume the last argument is an input file.  Load it.

    if      (extcmp(argv[argc - 1], ".jpg") == 0) p = jpg_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".png") == 0) p = png_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".tif") == 0) p = tif_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".img") == 0) p = pds_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".lbl") == 0) p = pds_load(argv[argc - 1]);

    // Process the output.

    if (p)
    {
        p->lat0  = lat0  * M_PI / 180.0;
        p->lat1  = lat1  * M_PI / 180.0;
        p->lon0  = lon0  * M_PI / 180.0;
        p->lon1  = lon1  * M_PI / 180.0;
        p->dlat0 = dlat0 * M_PI / 180.0;
        p->dlat1 = dlat1 * M_PI / 180.0;
        p->dlon0 = dlon0 * M_PI / 180.0;
        p->dlon1 = dlon1 * M_PI / 180.0;
        s = scm_ofile(o, n, p->c, b ? b : p->b, g ? g : p->g, t);
    }
    if (s && p)
        process(s, p, d);

    scm_minmax(s);
    scm_close(s);
    img_close(p);

    return 0;
}

//------------------------------------------------------------------------------
