// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_PROCESS_H
#define SCMTIFF_PROCESS_H

//------------------------------------------------------------------------------

int normal (int, char **, const char *, const float *);
int finish (int, char **);
int border (int, char **, const char *);
int mipmap (int, char **, const char *);
int combine(int, char **, const char *, const char *);
int convert(int, char **, const char *,
            int, int, int, int, int, const double *, const double *, const float *);

//------------------------------------------------------------------------------

#endif
