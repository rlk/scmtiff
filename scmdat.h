// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_SCMDAT_H
#define SCMTIFF_SCMDAT_H

#include <stdint.h>

//------------------------------------------------------------------------------
// The following structures define the format of an SCM TIFF: a BigTIFF with a
// specific set of seventeen fields in each IFD. LibTIFF4 has no trouble reading
// this format, but its SubIFD API is not sufficient to write it.

typedef struct header header;
typedef struct field  field;
typedef struct ifd    ifd;

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

struct ifd
{
    uint64_t count;

    field subfile_type;         // 0x00FE
    field image_width;          // 0x0100
    field image_length;         // 0x0101
    field bits_per_sample;      // 0x0102
    field compression;          // 0x0103
    field interpretation;       // 0x0106
    field description;          // 0x010E
    field strip_offsets;        // 0x0111 *
    field orientation;          // 0x0112
    field samples_per_pixel;    // 0x0115
    field rows_per_strip;       // 0x0116
    field strip_byte_counts;    // 0x0117 *
    field configuration;        // 0x011C
    field predictor;            // 0x013D
    field sub_ifds;             // 0x014A *
    field sample_format;        // 0x0153
    field page_index;           // 0xFFB1 *

    uint64_t next;
    uint64_t sub[4];
};                              // * Per-page variant fields

#pragma pack(pop)

//------------------------------------------------------------------------------

struct scm
{
    FILE *fp;                   // I/O file pointer
    char *str;                  // Description text

    int n;                      // Page sample count
    int c;                      // Sample channel count
    int b;                      // Channel bit count
    int g;                      // Channel signed flag
    int r;                      // Rows per strip

    ifd D;                      // IFD template

    uint8_t **binv;             // Strip bin scratch buffer pointers
    uint8_t **zipv;             // Strip zip scratch buffer pointers
};

typedef struct scm scm;

//------------------------------------------------------------------------------

void set_header(header *);
void set_field (field *, uint16_t, uint16_t, uint64_t, uint64_t);

int is_header(header *);
int is_ifd   (ifd *);

//------------------------------------------------------------------------------

size_t tifsizeof(uint16_t);

uint64_t scm_pint(scm *);
uint16_t scm_form(scm *);
uint16_t scm_type(scm *);
uint64_t scm_hdif(scm *);

void ftob(void *, const float *, size_t, int, int);
void btof(const void *, float *, size_t, int, int);

void enhdif(void *p, int, int, int);
void dehdif(void *p, int, int, int);

//------------------------------------------------------------------------------

#endif

