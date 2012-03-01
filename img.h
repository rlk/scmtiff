// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_IMG_H
#define SCMTIFF_IMG_H

//------------------------------------------------------------------------------

typedef struct img img;

struct img
{
    // Data buffer and parameters

    void  *p;
    void  *q;
    int    w;
    int    h;
    int    c;
    int    b;
    int    g;
    int    d;
    size_t n;

    float dnorm;
    float knorm;

    // Projection parameters

    float latmax;
    float latmin;
    float latp;
    float lonmax;
    float lonmin;
    float lonp;
    float l0;
    float s0;
    float res;
    float scale;
    float radius;
    float scaling_factor;

    // Mask parameters

    float lat0, lat1, dlat0, dlat1;
    float lon0, lon1, dlon0, dlon1;

    void (*project)(img *, float, float, float *);
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

void *img_scanline(img *, int);
int   img_sample  (img *, const float *, float *);
int   img_locate  (img *, const float *);

void  img_equirectangular (img *, float, float, float *);
void  img_orthographic    (img *, float, float, float *);
void  img_stereographic   (img *, float, float, float *);
void  img_cylindrical     (img *, float, float, float *);
void  img_default         (img *, float, float, float *);

//------------------------------------------------------------------------------

#endif
