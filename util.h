// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_UTIL_H
#define SCMTIFF_UTIL_H

//------------------------------------------------------------------------------

void   mid2(double *, const double *, const double *);
void   mid4(double *, const double *, const double *,
                      const double *, const double *);

double lerp1(double, double, double);
double lerp2(double, double, double, double, double, double);

void   slerp1(double *, const double *, const double *, double);
void   slerp2(double *, const double *, const double *,
                        const double *, const double *, double, double);

int extcmp(const char *, const char *);

//------------------------------------------------------------------------------

#endif