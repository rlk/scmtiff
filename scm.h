// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

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
int scm_get_l(scm *);

void scm_get_sample_corners(int, long, long, long, double *);
void scm_get_sample_center (int, long, long, long, double *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

long long scm_rewind(scm *);
long long scm_append(scm *, long long, long long, const float *);
long long scm_repeat(scm *, long long,
					 scm *, long long);

long long scm_read_node(scm *, long long, long long *, long long *);
bool      scm_read_page(scm *, long long, float *);

//------------------------------------------------------------------------------
// SCM TIFF metadata search.

void scm_sort_catalog(scm *);
bool scm_scan_catalog(scm *);
bool scm_make_catalog(scm *);
bool scm_make_extrema(scm *);

long long scm_find_index (scm *, long long);
long long scm_find_offset(scm *, long long);

long long scm_get_index  (scm *, long long);
long long scm_get_offset (scm *, long long);

//------------------------------------------------------------------------------

#endif
