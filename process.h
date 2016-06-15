// SCMTIFF Copyright (C) 2012-2015 Robert Kooima
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

#ifndef SCMTIFF_PROCESS_H
#define SCMTIFF_PROCESS_H

//------------------------------------------------------------------------------

int convert(int, char **, const char *, int, int, int, int, int,
           const float *, const double *, const double *, const double *);

int combine(int, char **, const char *, const char *);
int normal (int, char **, const char *, const float *);
int mipmap (int, char **, const char *, const char *, int);
int border (int, char **, const char *);
int prune  (int, char **, const char *);
int finish (int, char **, const char *, int);
int polish (int, char **);

int sample (int, char **, const float *, int);
int extrema(int, char **);
int query  (int, char **);

int rectify(int, char **, const char *, int,
           const float *, const double *, const double *, const double *);

//------------------------------------------------------------------------------

#endif
