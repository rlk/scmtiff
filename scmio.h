// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#ifndef SCMTIFF_SCMIO_H
#define SCMTIFF_SCMIO_H

//------------------------------------------------------------------------------

int    scm_alloc(scm *);

off_t  scm_write(scm *, const void *, size_t);
off_t  scm_align(scm *);

//------------------------------------------------------------------------------

int    scm_read_header  (scm *, header *);
int    scm_write_header (scm *, header *);

int    scm_read_field   (scm *, field *,       void *);
int    scm_write_field  (scm *, field *, const void *);

int    scm_read_ifd     (scm *, ifd *, off_t);
int    scm_write_ifd    (scm *, ifd *, off_t);

int    scm_read_preface (scm *);
int    scm_write_preface(scm *, const char *);

size_t scm_read_data    (scm *,       double *, size_t);
size_t scm_write_data   (scm *, const double *);

//------------------------------------------------------------------------------

int    scm_link_list(scm *, off_t, off_t);
int    scm_link_tree(scm *, off_t, off_t, int);

//------------------------------------------------------------------------------

#endif
