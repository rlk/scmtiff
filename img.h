// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_IMG_H
#define SCMTIFF_IMG_H

//------------------------------------------------------------------------------

typedef struct img img;

struct img
{
    void *p;
    int   w;
    int   h;
    int   c;
    int   b;
    int   s;

    void (*get)(img *, int, int, double *);

    void (*sample)(img *, const double *, double *);
};

//------------------------------------------------------------------------------

img *jpg_load(const char *);
img *png_load(const char *);
img *tif_load(const char *);
img *img_load(const char *);
img *lbl_load(const char *);

img *img_alloc(int, int, int, int, int);
void img_close(img *);

//------------------------------------------------------------------------------

void *img_scanline(img *, int);

void img_sample_spheremap(img *, const double *, double *);
void img_sample_test     (img *, const double *, double *);

//------------------------------------------------------------------------------

#endif
