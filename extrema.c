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
#include <float.h>

#include "img.h"
#include "util.h"

//------------------------------------------------------------------------------

int extrema(int argc, char **argv)
{
    img *p = NULL;

    // Iterate over all input file arguments.

    for (int i = 0; i < argc; i++)
    {
        const char *in = argv[i];

        // Load the input file.

        if      (extcmp(in, ".jpg") == 0) p = jpg_load(in);
        else if (extcmp(in, ".png") == 0) p = png_load(in);
        else if (extcmp(in, ".tif") == 0) p = tif_load(in);
        else if (extcmp(in, ".img") == 0) p = pds_load(in);
        else if (extcmp(in, ".lbl") == 0) p = pds_load(in);

        // Estimate the extrema.

        if (p)
        {
            const size_t w = (size_t) p->w;
            const size_t h = (size_t) p->h;
            const size_t n = 1024;

            float a =  FLT_MAX;
            float z = -FLT_MAX;
            float c[4];

            for (size_t s = 0; s < w * h; s += w * h / n)
            {
                if (img_pixel(p, (int) (s / w), (int) (s % w), c))
                {
                    switch (p->c)
                    {
                        case 4: a = min(a, c[3]); z = max(z, c[3]);
                        case 3: a = min(a, c[2]); z = max(z, c[2]);
                        case 2: a = min(a, c[1]); z = max(z, c[1]);
                        case 1: a = min(a, c[0]); z = max(z, c[0]);
                    }
                }
            }

            printf("%f\t%f\n", a, z);

            img_close(p);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------

