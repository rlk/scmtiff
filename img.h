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

    // Mask parameters

    double lat0, lat1, dlat0, dlat1;
    double lon0, lon1, dlon0, dlon1;

    double (*sample)(img *, double, double, double *);
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

void  *img_scanline(img *, int);
double img_sample(img *, const double *, double *);

double img_equirectangular (img *, double, double, double *);
double img_orthographic    (img *, double, double, double *);
double img_stereographic   (img *, double, double, double *);
double img_cylindrical     (img *, double, double, double *);
double img_default         (img *, double, double, double *);

//------------------------------------------------------------------------------

#endif
