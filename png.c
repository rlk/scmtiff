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
#include <png.h>

#include "img.h"
#include "err.h"

//------------------------------------------------------------------------------

img *png_load(const char *name)
{
    png_structp rp = 0;
    png_infop   ip = 0;
    FILE       *fp = 0;
    img         *p = 0;

    if ((fp = fopen(name, "rb")))
    {
        if ((rp = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0)) &&
            (ip = png_create_info_struct(rp)))
        {
            if (setjmp(png_jmpbuf(rp)) == 0)
            {
                png_init_io  (rp, fp);
                png_read_info(rp, ip);
                png_set_swap (rp);

                if (png_get_interlace_type(rp, ip) == PNG_INTERLACE_NONE)
                {
                    if ((p = img_alloc((int) png_get_image_width (rp, ip),
                                       (int) png_get_image_height(rp, ip),
                                       (int) png_get_channels    (rp, ip),
                                       (int) png_get_bit_depth   (rp, ip), 0)))

                        for (int i = 0; i < p->h; ++i)
                            png_read_row(rp, (png_bytep) img_scanline(p, i), 0);
                }
                else apperr("%s interlace not supported.", name);
            }
            png_destroy_read_struct(&rp, &ip, NULL);
        }
        fclose(fp);
    }
    return p;
}

//------------------------------------------------------------------------------
