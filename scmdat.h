// SCMTIFF Copyright (C) 2012 Robert Kooima
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

#ifndef SCMTIFF_SCMDAT_H
#define SCMTIFF_SCMDAT_H

#include <stdint.h>
#include <stdbool.h>

//------------------------------------------------------------------------------
// The following structures define the format of an SCM TIFF: a BigTIFF with a
// specific set of fields in each IFD. LibTIFF4 has no trouble handling this,
// Though our usage of 0x129 PageNumber is non-standard.

typedef struct header header;
typedef struct field  field;
typedef struct hfd    hfd;
typedef struct ifd    ifd;

#define SCM_HFD_COUNT    13
#define SCM_IFD_COUNT    14
#define SCM_PAGE_INDEX   0xFFB1
#define SCM_PAGE_OFFSET  0xFFB2
#define SCM_PAGE_MINIMUM 0xFFB3
#define SCM_PAGE_MAXIMUM 0xFFB4

#pragma pack(push)
#pragma pack(2)

struct header
{
    uint16_t endianness;
    uint16_t version;
    uint16_t offsetsize;
    uint16_t zero;
    uint64_t first_ifd;
};

struct field
{
    uint16_t tag;
    uint16_t type;
    uint64_t count;
    uint64_t offset;
};

struct hfd
{
    uint64_t count;

    field image_width;          // 0x0100
    field image_length;         // 0x0101
    field bits_per_sample;      // 0x0102
    field description;          // 0x010E
    field strip_offsets;        // 0x0111
    field samples_per_pixel;    // 0x0115
    field rows_per_strip;       // 0x0116
    field strip_byte_counts;    // 0x0117
    field sample_format;        // 0x0153
    field page_index;           // SCM_PAGE_INDEX
    field page_offset;          // SCM_PAGE_OFFSET
    field page_minimum;         // SCM_PAGE_MINIMUM
    field page_maximum;         // SCM_PAGE_MAXIMUM

    uint64_t next;
};

struct ifd
{
    uint64_t count;

    field image_width;          // 0x0100 * Required, uniform across all pages.
    field image_length;         // 0x0101 *
    field bits_per_sample;      // 0x0102 *
    field compression;          // 0x0103 *
    field interpretation;       // 0x0106 *
    field strip_offsets;        // 0x0111
    field orientation;          // 0x0112 *
    field samples_per_pixel;    // 0x0115 *
    field rows_per_strip;       // 0x0116 *
    field strip_byte_counts;    // 0x0117
    field configuration;        // 0x011C *
    field page_number;          // 0x0129
    field predictor;            // 0x013D *
    field sample_format;        // 0x0153 *

    uint64_t next;
};

#pragma pack(pop)

//------------------------------------------------------------------------------

typedef struct { long long x; long long o; } scm_pair;

struct scm
{
    FILE *fp;                   // I/O file pointer

    int n;                      // Page sample count
    int c;                      // Sample channel count
    int b;                      // Channel bit count
    int g;                      // Channel signed flag
    int r;                      // Rows per strip

    long long  xc;
    long long *xv;
    long long  oc;
    long long *ov;

    uint8_t **binv;             // Strip bin scratch buffer pointers
    uint8_t **zipv;             // Strip zip scratch buffer pointers
};

typedef struct scm scm;

//------------------------------------------------------------------------------

bool is_header(header *);
bool is_hfd   (hfd *);
bool is_ifd   (ifd *);

//------------------------------------------------------------------------------

size_t tifsizeof(uint16_t);

uint64_t scm_pint(scm *);
uint16_t scm_form(scm *);
uint16_t scm_type(scm *);
uint64_t scm_hdif(scm *);

//------------------------------------------------------------------------------

void ftob(void *, const float *, size_t, int, int);
void btof(const void *, float *, size_t, int, int);

void enhdif(void *, int, int, int);
void dehdif(void *, int, int, int);

void   tobin(scm *, uint8_t *, const float *, int);
void frombin(scm *, const uint8_t *, float *, int);

void   todif(scm *s, uint8_t *, int);
void fromdif(scm *s, uint8_t *, int);

void   tozip(scm *, uint8_t *, int, uint8_t *, uint32_t *);
void fromzip(scm *, uint8_t *, int, uint8_t *, uint32_t);

//------------------------------------------------------------------------------

#endif

