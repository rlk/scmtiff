// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_IMG_H
#define SCMTIFF_IMG_H

//------------------------------------------------------------------------------

typedef struct img img;

struct img
{
    // Data buffer and parameters

    void  *p;
    int    w;
    int    h;
    int    c;
    int    b;
    int    s;
    int    d;
    size_t n;

    // Projection parameters

    double latmax;
    double latmin;
    double latp;
    double lonmax;
    double lonmin;
    double lonp;
    double l0;
    double s0;
    double res;
    double scale;
    double radius;

    int (*sample)(img *, const double *, double *);
};

//------------------------------------------------------------------------------

img *jpg_load(const char *);
img *png_load(const char *);
img *tif_load(const char *);
img *pds_load(const char *);

img *img_mmap (int, int, int, int, int, const char *, off_t);
img *img_alloc(int, int, int, int, int);
void img_close(img *);

//------------------------------------------------------------------------------

void *img_scanline(img *, int);

int img_equirectangular (img *, const double *, double *);
int img_orthographic    (img *, const double *, double *);
int img_stereographic   (img *, const double *, double *);
int img_cylindrical     (img *, const double *, double *);
int img_default         (img *, const double *, double *);
int img_test            (img *, const double *, double *);

//------------------------------------------------------------------------------

#endif
