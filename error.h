// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_ERROR_H
#define SCMTIFF_ERROR_H

//------------------------------------------------------------------------------

void set_executable(const char *);

void app_err_exit(const char *, ...);
void sys_err_exit(const char *, ...);

void app_err_mesg(const char *, ...);
void sys_err_mesg(const char *, ...);
void app_wrn_mesg(const char *, ...);
void sys_wrn_mesg(const char *, ...);
void app_log_mesg(const char *, ...);
void sys_log_mesg(const char *, ...);

//------------------------------------------------------------------------------

#endif
