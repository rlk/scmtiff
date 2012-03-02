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

static void quincunx(float *w, const float *v)
{
    mid4(w + 12, v +  0, v +  3, v +  6, v +  9);
    mid2(w +  9, w + 12, v +  9);
    mid2(w +  6, w + 12, v +  6);
    mid2(w +  3, w + 12, v +  3);
    mid2(w +  0, w + 12, v +  0);
}

// Compute the corner vectors of the pixel at row i column j of the n-by-n page
// with corners c. Sample that pixel by projection into image p using a quincunx
// filtering pattern. This do-it-all function forms the kernel of the OpenMP
// parallelization.

static int sample(img *p, int i, int j, int n, const float *c, float *x)
{
    float v[12];
    float w[15];
    int   A = 0;

    scm_get_samp_corners(c, i, j, n, v);

    quincunx(w, v);

    for (int l = 4; l >= 0; --l)
    {
        float t[4];
        int a;

        if ((a = img_sample(p, w + l * 3, t)))
        {
            switch (p->c)
            {
                case 4: x[3] += t[3];
                case 3: x[2] += t[2];
                case 2: x[1] += t[1];
                case 1: x[0] += t[0];
            }
            A += a;
        }
    }
    if (A)
    {
        switch (p->c)
        {
            case 4: x[3] /= A;
            case 3: x[2] /= A;
            case 2: x[1] /= A;
            case 1: x[0] /= A;
        }
        return 1;
    }
    return 0;
}

// Determine whether the image intersects the four corners of a page.
// This function is inefficient and only works under limited circumstances.

int overlap(img *p, int n, const float *v)
{
    for     (int i = 0; i <= n; ++i)
        for (int j = 0; j <= n; ++j)
        {
            float u[3];

            scm_get_samp_vector(v, i, j, n, u);

            if (img_locate(p, u))
                return 1;
        }

    return 0;
}

// Consider page x of SCM s. Determine whether it contains any of image p.
// If so, sample it or recursively subdivide it as needed.

off_t divide(scm *s, off_t b, img *p, int x, int d, const float *v, float *o)
{
    off_t B = b;

    if (overlap(p, scm_get_n(s), v + x * 12))
    {
        if (d == 0)
        {
            const int N = scm_get_n(s) + 2;
            const int n = scm_get_n(s);
            const int c = scm_get_c(s);
            int k = 0;
            int j;
            int i;

            memset(o, 0, N * N * c * sizeof (float));

            #pragma omp parallel for private(j) reduction(+:k)
            for     (i = 0; i < n; ++i)
                for (j = 0; j < n; ++j)
                    k += sample(p, i, j, n, v + x * 12,
                                            o + c * (N * (i + 1) + (j + 1)));

            if (k) B = scm_append(s, B, x, o);
        }
        else
        {
            B = divide(s, B, p, scm_get_page_child(x, 0), d - 1, v, o);
            B = divide(s, B, p, scm_get_page_child(x, 1), d - 1, v, o);
            B = divide(s, B, p, scm_get_page_child(x, 2), d - 1, v, o);
            B = divide(s, B, p, scm_get_page_child(x, 3), d - 1, v, o);
        }
    }
    return B;
}

// Convert image p to SCM s at depth d. Allocate working buffers, precompute
// page corners, and perform a depth-first traversal of the page tree.

int process(scm *s, img *p, int d)
{
    const int m = scm_get_page_count(d);
    const int N = scm_get_n(s) + 2;
    const int c = scm_get_c(s);

    float *v;
    float *o;

    if ((v = (float *) calloc(m * 12,    sizeof (float))) &&
        (o = (float *) calloc(N * N * c, sizeof (float))))
    {
        off_t b = 0;

        scm_get_page_corners(d, v);

        b = divide(s, b, p, 0, d, v, o);
        b = divide(s, b, p, 1, d, v, o);
        b = divide(s, b, p, 2, d, v, o);
        b = divide(s, b, p, 3, d, v, o);
        b = divide(s, b, p, 4, d, v, o);
        b = divide(s, b, p, 5, d, v, o);

        free(o);
        free(v);
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
            // Set the channel format overrides.

            if (b == 0) b = p->b;
            if (g == 0) g = p->g;

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
                process(s, p, d);
                scm_close(s);
            }
            img_close(p);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
