// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_IMG_H
#define SCMTIFF_IMG_H

//------------------------------------------------------------------------------

typedef struct img img;

img *jpg_load(const char *);
img *png_load(const char *);
img *tif_load(const char *);
img *img_load(const char *);
img *lbl_load(const char *);

void img_close(img *);

//------------------------------------------------------------------------------

void img_sample(const double *);

int img_get_c(img *);
int img_get_b(img *);
int img_get_s(img *);

//------------------------------------------------------------------------------

#endif
