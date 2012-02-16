// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <math.h>
#include <string.h>

//------------------------------------------------------------------------------
// 2-vector and 4-vector normalized midpoint vector calculators

void mid2(double *m, const double *a, const double *b)
{
    m[0] = a[0] + b[0];
    m[1] = a[1] + b[1];
    m[2] = a[2] + b[2];

    double k = 1.0 / sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);

    m[0] *= k;
    m[1] *= k;
    m[2] *= k;
}

void mid4(double *m, const double *a, const double *b,
                     const double *c, const double *d)
{
    m[0] = a[0] + b[0] + c[0] + d[0];
    m[1] = a[1] + b[1] + c[1] + d[1];
    m[2] = a[2] + b[2] + c[2] + d[2];

    double k = 1.0 / sqrt(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);

    m[0] *= k;
    m[1] *= k;
    m[2] *= k;
}

//------------------------------------------------------------------------------
// 1D and 2D linear interpolation

double lerp1(double a, double b, double t)
{
    return b * t + a * (1 - t);
}

double lerp2(double a, double b, double c, double d, double s, double t)
{
    return lerp1(lerp1(a, b, t), lerp1(c, d, t), s);
}

//------------------------------------------------------------------------------
// 1D and 2D spherical linear interpolation

void slerp1(double *a, const double *b, const double *c, double t)
{
    const double k = acos(b[0] * c[0] + b[1] * c[1] + b[2] * c[2]);
    const double u = sin(k - t * k) / sin(k);
    const double v = sin(    t * k) / sin(k);

    a[0] = b[0] * u + c[0] * v;
    a[1] = b[1] * u + c[1] * v;
    a[2] = b[2] * u + c[2] * v;
}

void slerp2(double *v, const double *a, const double *b,
                       const double *c, const double *d, double x, double y)
{
    double t[3];
    double u[3];

    slerp1(t, a, b, x);
    slerp1(u, c, d, x);
    slerp1(v, t, u, y);
}

//------------------------------------------------------------------------------

int extcmp(const char *name, const char *ext)
{
    return strcasecmp(name + strlen(name) - strlen(ext), ext);
}

//------------------------------------------------------------------------------
