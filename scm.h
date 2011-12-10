// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_SCM_H
#define SCMTIFF_SCM_H

//------------------------------------------------------------------------------

typedef struct scm scm;

//------------------------------------------------------------------------------
// SCM TIFF file open/close.

scm *scm_ofile(const char *, int, int, int, int, const char *);
scm *scm_ifile(const char *);

void scm_close(scm *);

//------------------------------------------------------------------------------
// SCM TIFF file read/write.

off_t  scm_append(scm *, off_t, off_t, int, int, const double *);
off_t  scm_rewind(scm *);

int    scm_read_node(scm *, off_t, off_t *, off_t *);
size_t scm_read_page(scm *, off_t, double *);

int    scm_catalog(scm *, int **, off_t **);

//------------------------------------------------------------------------------
// SCM TIFF parameter queries

const char *scm_get_copyright(scm *);

int scm_get_n(scm *);
int scm_get_c(scm *);
int scm_get_b(scm *);
int scm_get_s(scm *);

double *scm_alloc_buffer(scm *);

//------------------------------------------------------------------------------
// SCM TIFF breadth-first page index relationships

int  scm_get_page_level  (int);
int  scm_get_page_count  (int);
int  scm_get_page_child  (int, int);
int  scm_get_page_parent (int);
int  scm_get_page_order  (int);
void scm_get_page_corners(int, double *);

//------------------------------------------------------------------------------

#endif
