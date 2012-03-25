// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

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
