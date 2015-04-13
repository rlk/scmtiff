// SCMTIFF Copyright (C) 2012-2015 Robert Kooima
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
#include <tiffio.h>
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

static double lerp(double a, double b, double t)
{
    return b * t + a * (1.0 - t);
}

// Sample the image at the given latitude and longitude. If it's a hit, add
// the value to the pixel at b.

static int accumulate(img *p, double lat, double lon, float *b, int c)
{
    double v[3];
    float  t[4];

    v[0] = sin(lon) * cos(lat);
    v[1] =            sin(lat);
    v[2] = cos(lon) * cos(lat);

    if (img_sample(p, v, t))
    {
        switch (c)
        {
            case 4: b[3] += t[3];
            case 3: b[2] += t[2];
            case 2: b[1] += t[1];
            case 1: b[0] += t[0];
        }
        return 1;
    }
    return 0;
}

// Perform a quincunx multisampling of the image at pixel (i, j) in the given
// range of latitude and longitude. This function forms the kernel of an OpenMP
// parallelization of the sampling of an entire TIFF tile.

static void dosamp(img *p, double lat0, double lat1,
                           double lon0, double lon1,
                           float *b, int i, int j, int s, int c)
{
    double i0 = lerp(lat0, lat1, ((float) i + 0.25) / s);
    double i1 = lerp(lat0, lat1, ((float) i + 0.50) / s);
    double i2 = lerp(lat0, lat1, ((float) i + 0.75) / s);

    double j0 = lerp(lon0, lon1, ((float) j + 0.25) / s);
    double j1 = lerp(lon0, lon1, ((float) j + 0.50) / s);
    double j2 = lerp(lon0, lon1, ((float) j + 0.75) / s);

    float *q = b + c * (i * s + j);

    int d = accumulate(p, i0, j0, q, c) +
            accumulate(p, i0, j2, q, c) +
            accumulate(p, i1, j1, q, c) +
            accumulate(p, i2, j0, q, c) +
            accumulate(p, i2, j2, q, c);

    if (d)
        switch (c)
        {
            case 4: q[3] /= d;
            case 3: q[2] /= d;
            case 2: q[1] /= d;
            case 1: q[0] /= d;
        }
}

// Sample the image across an entire TIFF tile covering the given range of
// latitude and longitude.

static void dotile(img *p, double lat0, double lat1,
                           double lon0, double lon1,
                           float *b, int s, int c)
{
    int i;
    int j;

    #pragma omp parallel for private(j)
    for     (i = 0; i < s; ++i)
        for (j = 0; j < s; ++j)
            dosamp(p, lat0, lat1, lon0, lon1, b, i, j, s, c);
}

// Sample the image across the entire sphere, storing the results to a tiled
// TIFF file with image size 2n x n, tile size s x s, and c channels.

static void process(const char *out, img *p, int n, int s, int c)
{
    TIFF  *T;
    float *b;

    if ((T = TIFFOpen(out, "w8")))
    {
        TIFFSetField(T, TIFFTAG_IMAGEWIDTH,  2 * n);
        TIFFSetField(T, TIFFTAG_IMAGELENGTH,     n);
        TIFFSetField(T, TIFFTAG_BITSPERSAMPLE,  32);
        TIFFSetField(T, TIFFTAG_SAMPLESPERPIXEL, c);
        TIFFSetField(T, TIFFTAG_SAMPLEFORMAT,    3);
        TIFFSetField(T, TIFFTAG_TILEWIDTH,       s);
        TIFFSetField(T, TIFFTAG_TILELENGTH,      s);
        TIFFSetField(T, TIFFTAG_PLANARCONFIG,    1);
        TIFFSetField(T, TIFFTAG_COMPRESSION,     8);
        TIFFSetField(T, TIFFTAG_PHOTOMETRIC, (c == 1) ? 1 : 2);

        if ((b = (float *) malloc(TIFFTileSize(T))))
        {
            int w = 2 * n / s;
            int h =     n / s;

            for     (int i = 0; i < h; ++i)
                for (int j = 0; j < w; ++j)
                {
                    memset(b, 0, TIFFTileSize(T));

                    dotile(p, -M_PI * (i    ) / h + 0.5 * M_PI,
                              -M_PI * (i + 1) / h + 0.5 * M_PI,
                         2.0 * M_PI * (j    ) / w -       M_PI,
                         2.0 * M_PI * (j + 1) / w -       M_PI, b, s, c);

                    TIFFWriteTile(T, b, j * s, i * s, 0, 0);
                }
        }

        TIFFClose(T);
    }
}

//------------------------------------------------------------------------------

int rectify(int argc, char **argv, const char *o,
                                           int n,
                                 const float  *N,
                                 const double *E,
                                 const double *L,
                                 const double *P)
{
    img  *p = NULL;
    const char *e = NULL;

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
            // Set the blending parameters.

            if (P[0] || P[1] || P[2])
            {
                p->latc = P[0] * M_PI / 180.0;
                p->lat0 = P[1] * M_PI / 180.0;
                p->lat1 = P[2] * M_PI / 180.0;
            }
            if (L[0] || L[1] || L[2])
            {
                p->lonc = L[0] * M_PI / 180.0;
                p->lon0 = L[1] * M_PI / 180.0;
                p->lon1 = L[2] * M_PI / 180.0;
            }

            // Set the equirectangular subset parameters.

            if (E[0] || E[1] || E[2] || E[3])
            {
                p->westernmost_longitude = E[0] * M_PI / 180.0;
                p->easternmost_longitude = E[1] * M_PI / 180.0;
                p->minimum_latitude = E[2] * M_PI / 180.0;
                p->maximum_latitude = E[3] * M_PI / 180.0;
            }

            // Set the normalization parameters.

            if (N[0] || N[1])
            {
                p->norm0 = N[0];
                p->norm1 = N[1];
            }
            else
            {
                p->norm0 = 0.0f;
                p->norm1 = 1.0f;
            }

            // Process the output.

            process(out, p, n, 256, p->c);
            img_close(p);
        }
    }
    return 0;
}

