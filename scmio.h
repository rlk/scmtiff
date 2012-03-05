// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_SCMIO_H
#define SCMTIFF_SCMIO_H

//------------------------------------------------------------------------------

int    scm_alloc(scm *);
void   scm_free (scm *);

int    scm_read (scm *,       void *, size_t, off_t);
off_t  scm_write(scm *, const void *, size_t);
off_t  scm_align(scm *);

//------------------------------------------------------------------------------

int scm_read_header  (scm *, header *);
int scm_write_header (scm *, header *);

int scm_read_field   (scm *, field *,       void *);
int scm_write_field  (scm *, field *, const void *);

int scm_read_ifd     (scm *, ifd *, off_t);
int scm_write_ifd    (scm *, ifd *, off_t);

int scm_read_preface (scm *);
int scm_write_preface(scm *, const char *);

int scm_read_cache (scm *, uint8_t **, uint64_t,   uint64_t,   uint16_t,
                                                   uint64_t *, uint32_t *);
int scm_write_cache(scm *, uint8_t **, uint64_t *, uint64_t *, uint16_t *,
                                                   uint64_t *, uint32_t *);

int scm_read_data (scm *,       float *, uint64_t,   uint64_t,   uint16_t);
int scm_write_data(scm *, const float *, uint64_t *, uint64_t *, uint16_t *);

//------------------------------------------------------------------------------

int scm_link_list(scm *, off_t, off_t);
int scm_link_tree(scm *, off_t, off_t, int);

//------------------------------------------------------------------------------

#endif
