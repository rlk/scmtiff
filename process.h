// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_PROCESS_H
#define SCMTIFF_PROCESS_H

//------------------------------------------------------------------------------

int sample (int, char **, const float *, int);
int normal (int, char **, const char *, const float *);
int finish (int, char **, const char *, int);
int border (int, char **, const char *);
int mipmap (int, char **, const char *);
int combine(int, char **, const char *, const char *);
int rectify(int, char **, const char *, int,
            const double *, const double *, const float *);
int convert(int, char **, const char *, int, int, int, int, int,
            const double *, const double *, const float *);

//------------------------------------------------------------------------------

#endif
