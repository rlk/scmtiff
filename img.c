// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>

#include "img.h"

//------------------------------------------------------------------------------

void img_close(img *p)
{
}

int img_get_c(img *p)
{
    return 0;
}

int img_get_b(img *p)
{
    return 0;
}

int img_get_s(img *p)
{
    return 0;
}

//------------------------------------------------------------------------------

void img_sample_test(img *p, const double *v, double *c)
{
    c[0] = (v[0] + 1.0) / 2.0;
    c[1] = (v[1] + 1.0) / 2.0;
    c[2] = (v[2] + 1.0) / 2.0;
}

//------------------------------------------------------------------------------
