// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

#include <unistd.h>
#include "scmdat.h"

//------------------------------------------------------------------------------
// SCM TIFF file open/close.

void scm_close(scm *);

scm *scm_ifile(const char *);
scm *scm_ofile(const char *, int, int, int, int, const char *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

off_t  scm_append(scm *, off_t, off_t, int, int, const double *);
off_t  scm_rewind(scm *);
void   scm_relink(scm *);
void   scm_minmax(scm *);

int    scm_read_node(scm *, off_t, off_t *, off_t *);
size_t scm_read_page(scm *, off_t, double *);

int    scm_mapping(scm *, off_t **);

//------------------------------------------------------------------------------
// SCM TIFF parameter queries

double *scm_alloc_buffer(scm *);

char *scm_get_description(scm *);

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_g(scm *);

void scm_get_min(scm *, double *);
void scm_get_max(scm *, double *);

//------------------------------------------------------------------------------
// SCM TIFF breadth-first page index relationships

int  scm_get_page_root   (int);
int  scm_get_page_depth  (int);
int  scm_get_page_count  (int);
int  scm_get_page_child  (int, int);
int  scm_get_page_parent (int);
int  scm_get_page_order  (int);
void scm_get_page_corners(int, double *);
void scm_get_page_neighbors(int, int *, int *, int *, int *);

//------------------------------------------------------------------------------

#endif
