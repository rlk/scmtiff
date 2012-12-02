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
#include <tiffio.h>

#include "img.h"
#include "err.h"

//------------------------------------------------------------------------------

img *tif_load(const char *name)
{
    img  *p = NULL;
    TIFF *T = NULL;

    TIFFSetWarningHandler(0);

    if ((T = TIFFOpen(name, "r")))
    {
        uint32 W, H;
        uint16 B, C, S;

        TIFFGetField(T, TIFFTAG_IMAGEWIDTH,      &W);
        TIFFGetField(T, TIFFTAG_IMAGELENGTH,     &H);
        TIFFGetField(T, TIFFTAG_BITSPERSAMPLE,   &B);
        TIFFGetField(T, TIFFTAG_SAMPLESPERPIXEL, &C);
        TIFFGetField(T, TIFFTAG_SAMPLEFORMAT,    &S);

        if ((p = img_alloc((int) W, (int) H, (int) C, (int) B, (S == 2))))

            for (uint32 i = 0; i < H; ++i)
                TIFFReadScanline(T, img_scanline(p, (int) i), i, 0);

        TIFFClose(T);
    }
    return p;
}

//------------------------------------------------------------------------------
