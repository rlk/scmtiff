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

void scm_get_sample_corners(int, long, long, long, double *);
void scm_get_sample_center (int, long, long, long, double *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

long long scm_rewind(scm *);
long long scm_append(scm *, long long, long long, const float *);
long long scm_repeat(scm *, long long, scm *, long long);
bool      scm_finish(scm *, int);

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
