// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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
