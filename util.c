// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <math.h>
#include <string.h>
#include <strings.h>

#include "util.h"

//------------------------------------------------------------------------------

void normalize(double *v)
{
    double k = 1.0 / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= k;
    v[1] *= k;
    v[2] *= k;
}

//------------------------------------------------------------------------------
// 2-vector and 4-vector normalized midpoint vector calculators

void mid2(double *m, const double *a, const double *b)
{
    m[0] = a[0] + b[0];
    m[1] = a[1] + b[1];
    m[2] = a[2] + b[2];

    normalize(m);
}

void mid4(double *m, const double *a, const double *b,
                    const double *c, const double *d)
{
    m[0] = a[0] + b[0] + c[0] + d[0];
    m[1] = a[1] + b[1] + c[1] + d[1];
    m[2] = a[2] + b[2] + c[2] + d[2];

    normalize(m);
}

//------------------------------------------------------------------------------
// 1D and 2D linear interpolation

float lerp1(float a, float b, float t)
{
    return b * t + a * (1.0f - t);
}

float lerp2(float a, float b, float c, float d, float s, float t)
{
    return lerp1(lerp1(a, b, t), lerp1(c, d, t), s);
}

//------------------------------------------------------------------------------

int extcmp(const char *name, const char *ext)
{
    return strcasecmp(name + strlen(name) - strlen(ext), ext);
}

//------------------------------------------------------------------------------
