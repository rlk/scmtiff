// SCMTIFF Copyright (C) 2012-2015 Robert Kooima
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

#ifndef SCMTIFF_SCMIO_H
#define SCMTIFF_SCMIO_H

#include <stdbool.h>

#ifdef __linux__
#define fseeko fseek
#define ftello ftell
#endif

//------------------------------------------------------------------------------

bool      scm_alloc(scm *);
void      scm_free (scm *);

bool      scm_ffwd (scm *);
bool      scm_seek (scm *,                       long long);
bool      scm_read (scm *,       void *, size_t, long long);
long long scm_write(scm *, const void *, size_t);
long long scm_align(scm *);

//------------------------------------------------------------------------------

void      scm_field(field *, uint16_t, uint16_t, uint64_t, uint64_t);

void      scm_init_header   (       header *);
bool      scm_read_header   (scm *, header *);
long long scm_write_header  (scm *, header *);

bool      scm_init_hfd      (scm *, hfd *);
bool      scm_read_hfd      (scm *, hfd *, long long);
long long scm_write_hfd     (scm *, hfd *, long long);

bool      scm_init_ifd      (scm *, ifd *);
bool      scm_read_ifd      (scm *, ifd *, long long);
long long scm_write_ifd     (scm *, ifd *, long long);

bool      scm_read_preamble (scm *);
bool      scm_write_preamble(scm *);

//------------------------------------------------------------------------------

bool scm_read_zips (scm *, uint8_t **, uint64_t,   uint64_t,   uint16_t,
                                                   uint64_t *, uint32_t *);
bool scm_write_zips(scm *, uint8_t **, uint64_t *, uint64_t *, uint16_t *,
                                                   uint64_t *, uint32_t *);

bool scm_read_data (scm *,       float *, uint64_t,   uint64_t,   uint16_t);
bool scm_write_data(scm *, const float *, uint64_t *, uint64_t *, uint16_t *);

//------------------------------------------------------------------------------

bool scm_link_list(scm *, long long, long long);

//------------------------------------------------------------------------------

#endif
