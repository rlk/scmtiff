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
#include <jpeglib.h>

#include "img.h"
#include "err.h"

//------------------------------------------------------------------------------

img *jpg_load(const char *name)
{
    img     *p = NULL;
    FILE    *s = NULL;
    JSAMPLE *b = NULL;

    if ((s = fopen(name, "rb")))
    {
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr         jerr;

        cinfo.err = jpeg_std_error(&jerr);

        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src        (&cinfo, s);
        jpeg_read_header      (&cinfo, TRUE);
        jpeg_start_decompress (&cinfo);

        if ((p = img_alloc((int) cinfo.output_width,
                           (int) cinfo.output_height,
                           (int) cinfo.output_components, 8, 0)))
        {
            while (cinfo.output_scanline < cinfo.output_height)
            {
                b = (JSAMPLE *) img_scanline(p, (int) cinfo.output_scanline);
                jpeg_read_scanlines(&cinfo, &b, 1);
            }
        }

        jpeg_finish_decompress (&cinfo);
        jpeg_destroy_decompress(&cinfo);

        fclose(s);
    }
    else syserr("Failed to open JPEG '%s'", name);

    return p;
}

//------------------------------------------------------------------------------
