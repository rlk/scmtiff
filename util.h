// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_UTIL_H
#define SCMTIFF_UTIL_H

//------------------------------------------------------------------------------

#define min(A, B) (((A) < (B)) ? (A) : (B))
#define max(A, B) (((A) > (B)) ? (A) : (B))

void  normalize(double *);

void  mid2(double *, const double *, const double *);
void  mid4(double *, const double *, const double *,
                    const double *, const double *);

float lerp1(float, float, float);
float lerp2(float, float, float, float, float, float);

int extcmp(const char *, const char *);

//------------------------------------------------------------------------------

#endif
