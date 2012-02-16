// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_ERROR_H
#define SCMTIFF_ERROR_H

#include <errno.h>

//------------------------------------------------------------------------------

void printerr(const char *, int, int, const char *, ...);

//------------------------------------------------------------------------------

void setexe(const char *);

#define apperr(...) printerr(__FILE__, __LINE__, 0,     __VA_ARGS__)
#define syserr(...) printerr(__FILE__, __LINE__, errno, __VA_ARGS__)

//------------------------------------------------------------------------------

#endif
