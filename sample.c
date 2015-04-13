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

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include "scm.h"
#include "scmdef.h"
#include "util.h"

//------------------------------------------------------------------------------

static bool traverse(scm *s, double a, double b, long long x, int d,
                     long long *h, float *p, float *q)
{
    long long i;

    if ((i = scm_search(s, x)) >= 0)
    {
        if (d > 0)
        {
            long long x0 = scm_page_child(x, 0);
            long long x1 = scm_page_child(x, 1);
            long long x2 = scm_page_child(x, 2);
            long long x3 = scm_page_child(x, 3);

            double a0 = 2.0 * (a - 0.0);
            double a1 = 2.0 * (a - 0.5);
            double b0 = 2.0 * (b - 0.0);
            double b1 = 2.0 * (b - 0.5);

            // Seek the deepest existing node that contains point (a, b).

            if (b < 0.5)
            {
                if (a < 0.5)
                {
                    if (traverse(s, a0, b0, x0, d - 1, h, p, q))
                        return true;
                }
                else
                {
                    if (traverse(s, a1, b0, x1, d - 1, h, p, q))
                        return true;
                }
            }
            else
            {
                if (a < 0.5)
                {
                    if (traverse(s, a0, b1, x2, d - 1, h, p, q))
                        return true;
                }
                else
                {
                    if (traverse(s, a1, b1, x3, d - 1, h, p, q))
                        return true;
                }
            }
        }

        // This must be the best page. Load it, if not already loaded.

        if (h[0] != i)
        {
            if (scm_read_page(s, scm_get_offset(s, i), p))
                h[0] = i;
        }

        // If loaded, sample this page at the given point.

        if (h[0] == i)
        {
            const int c = scm_get_c(s);
            const int n = scm_get_n(s);

            int j1 = (int) floor(a * n) + 1, j2 = j1 + 1;
            int i1 = (int) floor(b * n) + 1, i2 = i1 + 1;

            float jj = (float) (a * n - floor(a * n));
            float ii = (float) (b * n - floor(b * n));

            for (int k = 0; k < c; ++k)
                q[k] = lerp1(lerp1(p[((n + 2) * i1 + j1) * c + k],
                                   p[((n + 2) * i1 + j2) * c + k], jj),
                             lerp1(p[((n + 2) * i2 + j1) * c + k],
                                   p[((n + 2) * i2 + j2) * c + k], jj), ii);

            return true;
        }
    }
    return false;
}

static bool locate(scm *s, double lat, double lon, int d,
                   long long *h, float *p, float *q)
{
    int i = 4;

    double X, x = sin(lon) * cos(lat);
    double Y, y =            sin(lat);
    double Z, z = cos(lon) * cos(lat);

    double xx = fabs(x);
    double yy = fabs(y);
    double zz = fabs(z);

    if      (xx > yy && xx > zz)
    {
        if (x < 0) { X =  z; Y = -y; Z = -x; i = 1; }
        else       { X = -z; Y = -y; Z =  x; i = 0; }
    }
    else if (yy > xx && yy > zz)
    {
        if (y < 0) { X =  x; Y = -z; Z = -y; i = 3; }
        else       { X =  x; Y =  z; Z =  y; i = 2; }
    }
    else
    {
        if (z < 0) { X = -x; Y = -y; Z = -z; i = 5; }
        else       { X =  x; Y = -y; Z =  z; i = 4; }
    }

    double a = -atan2(X, Z);
    double b = -atan2(Y, Z);

    double A = (-a + M_PI / 4.0) / (M_PI / 2.0);
    double B = (-b + M_PI / 4.0) / (M_PI / 2.0);

    return traverse(s, A, B, i, d, h, p, q);
}

static void process(scm *s, const float *R, int d)
{
    float *p;

    if ((p = scm_alloc_buffer(s)))
    {
        long long h = -1;

        double lon;
        double lat;

        while (scanf("%lf %lf", &lat, &lon) == 2)
        {
            int c = scm_get_c(s);

            lat *= M_PI / 180.0;
            lon *= M_PI / 180.0;

            float q[4];

            if (locate(s, lat, lon, d, &h, p, q))
            {
                for (int i = 0; i < c; i++)
                    printf("%f ", q[i] * (R[1] - R[0]) + R[0]);
                printf("\n");
            }
        }
        free(p);
    }
}

#if 0
static void image(scm *s, const float *R, int d)
{
    float *p;
    float q[4];

    if ((p = scm_alloc_buffer(s)))
    {
        int h = -1;
        int i;
        int j;

        for     (i =   90; i > -90; i -= 1)
            for (j = -180; j < 180; j += 1)
            {
                double lon = ((double) j + 0.5) * M_PI / 180.0;
                double lat = ((double) i - 0.5) * M_PI / 180.0;

                locate(s, lat, lon, d, &h, p, q);

                unsigned char c = (unsigned char) (q[0] * 255);

                fwrite(&c, 1, 1, stdout);
            }

        free(p);
    }
}
#endif
//------------------------------------------------------------------------------

int sample(int argc, char **argv, const float *R, int d)
{
    if (argc > 0)
    {
        scm *s;

        if ((s = scm_ifile(argv[0])))
        {
            if (scm_scan_catalog(s))
            {
                process(s, R, d);
                // image(s, R, d);
            }
            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
