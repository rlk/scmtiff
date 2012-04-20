// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

#include <stdbool.h>
#include "scmdat.h"

//------------------------------------------------------------------------------
// SCM TIFF file open/close.

void scm_close(scm *);

scm *scm_ifile(const char *);
scm *scm_ofile(const char *, int, int, int, int, const char *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

void      scm_catalog(scm *);
long long scm_rewind(scm *);
long long scm_append(scm *, long long, long long, const float *);
long long scm_repeat(scm *, long long,
					 scm *, long long);

long long scm_read_node(scm *, long long, long long *, long long *);
bool      scm_read_page(scm *, long long, float *);

int scm_mapping(scm *, long long **);

//------------------------------------------------------------------------------

typedef struct { long long x; long long o; } scm_pair;

long long scm_read_catalog(scm *, scm_pair **);
long long scm_scan_catalog(scm *, scm_pair **);
void      scm_sort_catalog(scm_pair *, long long);
long long scm_seek_catalog(scm_pair *, long long, long long, long long);
void      scm_make_catalog(scm *s);
void      scm_make_extrema(scm *s);

//------------------------------------------------------------------------------
// SCM TIFF parameter queries

float *scm_alloc_buffer(scm *);

char *scm_get_description(scm *);

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_g(scm *);

void scm_get_sample_corners(int, long, long, long, double *);
void scm_get_sample_center (int, long, long, long, double *);

//------------------------------------------------------------------------------

#endif
