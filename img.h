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

#ifndef SCMTIFF_IMG_H
#define SCMTIFF_IMG_H

//------------------------------------------------------------------------------

typedef struct img img;

struct img
{
    // Data buffer and parameters

    void  *p;  // Pixel buffer pointer
    int    w;  // Image width
    int    h;  // Image height
    int    c;  // Image channel count
    int    b;  // Image bits per channel
    int    g;  // Image channel signedness
    int    o;  // Image channel byte order
    int    d;  // Memory-mapped file descriptor
    size_t n;  // Memory-mapped size
    void  *q;  // Memory-mapped pointer

    // Normalization parameters

    float norm0;
    float norm1;

    // PDS parameters

    double maximum_latitude;
    double minimum_latitude;
    double  center_latitude;
    double easternmost_longitude;
    double westernmost_longitude;
    double      center_longitude;
    double   line_projection_offset;
    double sample_projection_offset;
    double map_resolution;
    double map_scale;
    double a_axis_radius;

    float scaling_factor;
    float offset;

    // Blending parameters

    double latc, lat0, lat1;
    double lonc, lon0, lon1;

    int (*project)(img *, const double *, double, double, double *);
};

//------------------------------------------------------------------------------

img *jpg_load(const char *);
img *png_load(const char *);
img *tif_load(const char *);
img *pds_load(const char *);

img *img_alloc(int, int, int, int, int);
void img_close(img *);

void img_set_defaults(img *);

//------------------------------------------------------------------------------

int   img_pixel   (img *, int, int, float *);
void *img_scanline(img *, int);
int   img_sample  (img *, const double *, float *);
int   img_locate  (img *, const double *);

int   img_equirectangular (img *, const double *, double, double, double *);
int   img_orthographic    (img *, const double *, double, double, double *);
int   img_polar_stereographic   (img *, const double *, double, double, double *);
int   img_simple_cylindrical     (img *, const double *, double, double, double *);
int   img_default         (img *, const double *, double, double, double *);
int   img_scube           (img *, const double *, double, double, double *);

//------------------------------------------------------------------------------

#endif
