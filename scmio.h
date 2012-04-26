// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_SCMIO_H
#define SCMTIFF_SCMIO_H

#ifdef __linux__
#define fseeko fseek
#define ftello ftell
#endif

//------------------------------------------------------------------------------

int       scm_alloc(scm *);
void      scm_free (scm *);

int       scm_read (scm *,       void *, size_t, long long);
long long scm_write(scm *, const void *, size_t);
long long scm_align(scm *);

//------------------------------------------------------------------------------

void scm_init_header (       header *);
int  scm_read_header (scm *, header *);
int  scm_write_header(scm *, header *);

void scm_init_field  (       field *, uint16_t, uint16_t, uint64_t, uint64_t);
int  scm_read_field  (scm *, field *,       void *);
int  scm_write_field (scm *, field *, const void *);

int  scm_init_hfd    (scm *, hfd *);
int  scm_read_hfd    (scm *, hfd *, long long);
int  scm_write_hfd   (scm *, hfd *, long long);

int  scm_init_ifd    (scm *, ifd *);
int  scm_read_ifd    (scm *, ifd *, long long);
int  scm_write_ifd   (scm *, ifd *, long long);

int  scm_read_preamble  (scm *);
int  scm_rwrite_preamble(scm *, const char *);

//------------------------------------------------------------------------------

int scm_read_zips (scm *, uint8_t **, uint64_t,   uint64_t,   uint16_t,
                                                  uint64_t *, uint32_t *);
int scm_write_zips(scm *, uint8_t **, uint64_t *, uint64_t *, uint16_t *,
                                                  uint64_t *, uint32_t *);

int scm_read_data (scm *,       float *, uint64_t,   uint64_t,   uint16_t);
int scm_write_data(scm *, const float *, uint64_t *, uint64_t *, uint16_t *);

//------------------------------------------------------------------------------

int scm_link_list(scm *, long long, long long);

//------------------------------------------------------------------------------

#endif
