// SCMTIFF Copyright (C) 2012 Robert Kooima
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITH-
// OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

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

int grow(float *, float *, int, int);

//------------------------------------------------------------------------------

#endif
