// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "img.h"

//------------------------------------------------------------------------------

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

    slerp2(v + 0, u + 0, u + 3, u + 6, u + 9, r0, c0);
    slerp2(v + 3, u + 0, u + 3, u + 6, u + 9, r0, c1);
    slerp2(v + 6, u + 0, u + 3, u + 6, u + 9, r1, c0);
    slerp2(v + 9, u + 0, u + 3, u + 6, u + 9, r1, c1);
}

//------------------------------------------------------------------------------

int process(scm *s, img *p, int d)
{
    const int M = scm_get_page_count(d);
    const int n = scm_get_n(s);
    const int N = scm_get_n(s) + 2;
    const int C = scm_get_c(s);

    off_t  *cat;
    double *dat;
    double *vec;

    if ((vec = (double *) calloc(M, 12 * sizeof (double))))
    {
        if ((dat = (double *) calloc(N * N * C, sizeof (double))))
        {
            if ((cat = (off_t *) calloc(M, sizeof (off_t))))
            {
                int i;
                int r;
                int c;

                scm_get_page_corners(d, vec);

                for (i = 0; i < M; ++i)
                {
                    memset(dat, 0, N * N * C * sizeof (double));

                    for     (r = 0; r < n; ++r)
                        for (c = 0; c < n; ++c)
                        {
                            double v[12];
                            double w[15];
                            double t[20];

                            corner_vectors(v, vec + i * 12, r, c, n);
                            sample_vectors(w, v);

                            p->sample(t + 16, w + 12);
                            p->sample(t + 12, w +  9);
                            p->sample(t +  8, w +  6);
                            p->sample(t +  4, w +  3);
                            p->sample(t +  0, w +  0);

                            double *pix = dat + C * ((r + 1) * N + (c + 1));

                            if (C > 0) pix[0] = t[0] + t[4] + t[ 8] + t[12];
                            if (C > 1) pix[1] = t[1] + t[5] + t[ 9] + t[13];
                            if (C > 2) pix[2] = t[2] + t[6] + t[10] + t[14];
                            if (C > 3) pix[3] = t[3] + t[7] + t[11] + t[15];
                        }

                    cat[i] = scm_append(s, cat[i - 1],
                                           cat[scm_get_page_parent(i)],
                                               scm_get_page_order (i), dat);
                }
            }
            free(cat);
        }
        free(dat);
    }
    free(vec);

    return 0;
}

static int extcmp(const char *name, const char *ext)
{
    return strcasecmp(name + strlen(name) - strlen(ext), ext);
}

int convert(int argc, char **argv)
{
    const char *t = "Copyright (c) 2011 Robert Kooima";
    const char *o = "out.tif";
    int         n = 512;
    int         d = 0;

    img *p = NULL;
    scm *s = NULL;

    int i;

    for (i = 1; i < argc; ++i)
        if      (strcmp(argv[i], "-o") == 0) o =      argv[++i];
        else if (strcmp(argv[i], "-t") == 0) t =      argv[++i];
        else if (strcmp(argv[i], "-n") == 0) n = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d") == 0) d = atoi(argv[++i]);

    if      (extcmp(argv[argc - 1], ".jpg") == 0) p = jpg_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".png") == 0) p = png_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".tif") == 0) p = tif_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".img") == 0) p = img_load(argv[argc - 1]);
    else if (extcmp(argv[argc - 1], ".lbl") == 0) p = lbl_load(argv[argc - 1]);

    if (p)
        s = scm_ofile(o, n, p->c, p->b, p->s, t);

    if (s && p)
        process(s, p, d);

    scm_close(s);
    img_close(p);

    return 0;
}

//------------------------------------------------------------------------------
