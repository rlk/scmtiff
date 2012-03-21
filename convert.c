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

char *load_txt(const char *name)
{
    // Load the named file into a newly-allocated buffer.

    FILE *fp = 0;
    void  *p = 0;
    size_t n = 0;

    if ((fp = fopen(name, "rb")))
    {
        if (fseek(fp, 0, SEEK_END) == 0)
        {
            if ((n = (size_t) ftell(fp)))
            {
                if (fseek(fp, 0, SEEK_SET) == 0)
                {
                    if ((p = calloc(n + 1, 1)))
                    {
                        fread(p, 1, n, fp);
                    }
                }
            }
        }
        fclose(fp);
    }
    return p;
}

//------------------------------------------------------------------------------

// Given the four corner vectors of a sample, compute the five internal vectors
// of a quincunx filtering of that sample.

static void quincunx(float *q, const float *v)
{
    mid4(q + 12, v +  0, v +  3, v +  6, v +  9);
    mid2(q +  9, q + 12, v +  9);
    mid2(q +  6, q + 12, v +  6);
    mid2(q +  3, q + 12, v +  3);
    mid2(q +  0, q + 12, v +  0);
}

// Compute the corner vectors of the pixel at row i column j of the n-by-n page
// with corners c. Sample that pixel by projection into image p using a quincunx
// filtering pattern. This do-it-all function forms the kernel of the OpenMP
// parallelization.

static int sample(img *p, int f, int i, int j, int n,
                                 int u, int v, int w, float *o)
{
    float c[12];
    float q[15];
    int   D = 0;

    scm_get_sample_corners(f, n * u + i, n * v + j, n * w, c);
    quincunx(q, c);

    for (int l = 4; l >= 0; --l)
    {
        float t[4];
        int d;

        if ((d = img_sample(p, q + l * 3, t)))
        {
            switch (p->c)
            {
                case 4: o[3] += t[3];
                case 3: o[2] += t[2];
                case 2: o[1] += t[1];
                case 1: o[0] += t[0];
            }
            D += d;
        }
    }
    if (D)
    {
        switch (p->c)
        {
            case 4: o[3] /= D;
            case 3: o[2] /= D;
            case 2: o[1] /= D;
            case 1: o[0] /= D;
        }
        return 1;
    }
    return 0;
}

// Determine whether the image intersects the four corners of a page.
// This function is inefficient and only works under limited circumstances.

int overlap(img *p, int f, int u, int v, int w)
{
    const int n = 128;

    for     (int i = 0; i <= n; ++i)
        for (int j = 0; j <= n; ++j)
        {
            float c[3];

            scm_get_sample_center(f, n * u + i, n * v + j, n * w, c);

            if (img_locate(p, c))
                return 1;
        }

    return 0;
}

// Consider page x of SCM s. Determine whether it contains any of image p.
// If so, sample it or recursively subdivide it as needed.

off_t divide(scm *s, off_t b, int f, int x, int d,
                              int u, int v, int w, img *p, float *o)
{
    off_t a = b;

    if (overlap(p, f, u, v, w))
    {
        if (d == 0)
        {
            const int m = scm_get_n(s) + 2;
            const int n = scm_get_n(s);
            const int c = scm_get_c(s);

            int h = 0;
            int i;
            int j;

            memset(o, 0, m * m * c * sizeof (float));

            #pragma omp parallel for private(j) reduction(+:h)
            for     (i = 0; i < n; ++i)
                for (j = 0; j < n; ++j)
                    h += sample(p, f, i, j, n, u, v, w,
                                o + c * (m * (i + 1) + (j + 1)));

            if (h) a = scm_append(s, a, x, o);
        }
        else
        {
            int x0 = scm_get_page_child(x, 0);
            int x1 = scm_get_page_child(x, 1);
            int x2 = scm_get_page_child(x, 2);
            int x3 = scm_get_page_child(x, 3);

            a = divide(s, a, f, x0, d - 1, u * 2,     v * 2,     w * 2, p, o);
            a = divide(s, a, f, x1, d - 1, u * 2,     v * 2 + 1, w * 2, p, o);
            a = divide(s, a, f, x2, d - 1, u * 2 + 1, v * 2,     w * 2, p, o);
            a = divide(s, a, f, x3, d - 1, u * 2 + 1, v * 2 + 1, w * 2, p, o);
        }
    }
    return a;
}

// Convert image p to SCM s with depth d. Allocate working buffers and perform a
// depth-first traversal of the page tree.

int process(scm *s, int d, img *p)
{
    const int m = scm_get_n(s) + 2;
    const int c = scm_get_c(s);

    float *o;

    if ((o = (float *) calloc(m * m * c, sizeof (float))))
    {
        off_t b = 0;

        b = divide(s, b, 0, 0, d, 0, 0, 1, p, o);
        b = divide(s, b, 1, 1, d, 0, 0, 1, p, o);
        b = divide(s, b, 2, 2, d, 0, 0, 1, p, o);
        b = divide(s, b, 3, 3, d, 0, 0, 1, p, o);
        b = divide(s, b, 4, 4, d, 0, 0, 1, p, o);
        b = divide(s, b, 5, 5, d, 0, 0, 1, p, o);

        free(o);
    }
    return 0;
}

//------------------------------------------------------------------------------

int convert(int argc, char **argv, const char *o,
                                   const char *t,
                                           int n,
                                           int d,
                                           int b,
                                           int g,
                                           int x,
                                  const float *L,
                                  const float *P,
                                  const float *N)
{
    img  *p = NULL;
    scm  *s = NULL;
    char *T = NULL;
    char *e = NULL;

    char out[256];

    // Load the description text, if any.

    if (t == NULL || (T = load_txt(t)) == NULL)
        T = "Copyright (C) 2011 Robert Kooima";

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

            if (b == 0) b = p->b;
            if (g == 0) g = p->g;

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

            if ((s = scm_ofile(out, n, p->c, b, g, T)))
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
