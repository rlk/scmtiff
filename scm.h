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

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

#include <stdbool.h>
#include "scmdat.h"

//------------------------------------------------------------------------------
// SCM TIFF file open/close.

void scm_close(scm *);

scm *scm_ifile(const char *);
scm *scm_ofile(const char *, int, int, int, int);

//------------------------------------------------------------------------------
// SCM TIFF parameter queries

float *scm_alloc_buffer(scm *);

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_g(scm *);

void scm_get_sample_corners(int, long, long, long, double *);
void scm_get_sample_center (int, long, long, long, double *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

long long scm_rewind(scm *);
long long scm_append(scm *, long long, long long, const float *);
long long scm_repeat(scm *, long long, scm *, long long);
bool      scm_finish(scm *, const char *, int);
bool      scm_polish(scm *);

bool scm_read_page(scm *, long long, float *);

//------------------------------------------------------------------------------
// SCM TIFF metadata search.

bool scm_scan_catalog(scm *);

long long scm_get_length(scm *);
long long scm_get_index (scm *, long long);
long long scm_get_offset(scm *, long long);

long long scm_search(scm *, long long);

//------------------------------------------------------------------------------

#endif
