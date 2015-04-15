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

#ifndef SCMTIFF_CONFIG_H
#define SCMTIFF_CONFIG_H

#ifdef _WIN32
#include <windows.h>
#include <float.h>
#include "getopt.h"

#define strcasecmp(a, b) _stricmp(a, b)
#define isnormal(x) ((_fpclass(x) == _FPCLASS_PN) \
                  || (_fpclass(x) == _FPCLASS_NN) \
                  || (_fpclass(x) == _FPCLASS_PZ) \
                  || (_fpclass(x) == _FPCLASS_NZ))

static inline double now()
{
    LARGE_INTEGER c;
    LARGE_INTEGER f;
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    return (double) c.QuadPart / (double) f.QuadPart;
}

#else

#include <getopt.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>

static inline double now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + (double) tv.tv_usec / 1000000.0;
}

#endif
#endif
