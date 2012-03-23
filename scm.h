// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

#include "scmdat.h"

//------------------------------------------------------------------------------
// SCM TIFF file open/close.

void scm_close(scm *);

scm *scm_ifile(const char *);
scm *scm_ofile(const char *, int, int, int, int, const char *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

void      scm_relink(scm *);
long long scm_rewind(scm *);
long long scm_append(scm *, long long, long long, const float *);
long long scm_repeat(scm *, long long,
					 scm *, long long);

long long scm_read_node(scm *, long long, long long *, long long *);
size_t    scm_read_page(scm *, long long, float *);

int scm_mapping(scm *, long long **);

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// SCM TIFF parameter queries

float *scm_alloc_buffer(scm *);

char *scm_get_description(scm *);

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_g(scm *);

//------------------------------------------------------------------------------
// SCM TIFF breadth-first page index relationships

long long scm_get_page_count (long long);
long long scm_get_page_root  (long long);
int       scm_get_page_order (long long);
int       scm_get_page_depth (long long);
long long scm_get_page_parent(long long);
long long scm_get_page_child (long long, int);

void scm_get_page_neighbors(long long, long long *, long long *,
									   long long *, long long *);

void scm_get_sample_corners(int, long, long, long, double *);
void scm_get_sample_center (int, long, long, long, double *);

//------------------------------------------------------------------------------

#endif
