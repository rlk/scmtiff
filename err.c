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

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "err.h"

//------------------------------------------------------------------------------

static const char *executable = NULL;

void setexe(const char *str)
{
    executable = str;
}

void printerr(const char *file, int line, int err, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    {
        if (executable)
            fprintf(stderr, "%s (%s:%d) : ", executable, file, line);
        else
            fprintf(stderr,    "(%s:%d) : ",             file, line);

        if (fmt)
           vfprintf(stderr, fmt, ap);

        if (err)
            fprintf(stderr, " : %s\n", strerror(err));
        else
            fprintf(stderr, "\n");
    }
    va_end(ap);
}

//------------------------------------------------------------------------------
