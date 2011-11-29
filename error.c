// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

//------------------------------------------------------------------------------

static const char *executable = NULL;

void set_executable(const char *str)
{
    executable = str;
}

//------------------------------------------------------------------------------

static void do_error(va_list ap, const char *fmt, const char *tag, int no)
{
    if (executable)
        fprintf(stderr, "%s : ", executable);

    if (tag)
        fprintf(stderr, "%s : ", tag);

    if (fmt)
       vfprintf(stderr, fmt, ap);

    if (no)
        fprintf(stderr, " : %s\n", strerror(no));
    else
        fprintf(stderr, "\n");
}

//------------------------------------------------------------------------------

void app_err_exit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Error", 0);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void sys_err_exit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Error", errno);
    va_end(ap);
    exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------------

void app_err_mesg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Error", 0);
    va_end(ap);
}

void sys_err_mesg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Error", errno);
    va_end(ap);
}

int app_err_zero(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Error", 0);
    va_end(ap);
    return 0;
}

int sys_err_zero(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Error", errno);
    va_end(ap);
    return 0;
}

//------------------------------------------------------------------------------

void app_wrn_mesg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Warning", 0);
    va_end(ap);
}

void sys_wrn_mesg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, "Warning", errno);
    va_end(ap);
}

void app_log_mesg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, 0, 0);
    va_end(ap);
}

void sys_log_mesg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    do_error(ap, fmt, 0, errno);
    va_end(ap);
}

//------------------------------------------------------------------------------
