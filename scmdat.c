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

#include <stdint.h>
#include <stdio.h>
#include <zlib.h>

#include "scmdat.h"
#include "util.h"

//------------------------------------------------------------------------------

// We're very very picky about what constitutes an SCM TIFF. Essentially, it's
// not an SCM TIFF if it wasn't written by this TIFF writer. Check a TIFF header
// and IFD to confirm that it provides exactly the expected content.

bool is_header(header *hp)
{
    if (hp->endianness            != 0x4949)           return false;
    if (hp->version               != 0x002B)           return false;
    if (hp->offsetsize            != 0x0008)           return false;
    if (hp->zero                  != 0x0000)           return false;
    return true;
}

bool is_hfd(hfd *hp)
{
    if (hp->count                 != SCM_HFD_COUNT)    return false;
    if (hp->image_width.tag       != 0x0100)           return false;
    if (hp->image_length.tag      != 0x0101)           return false;
    if (hp->bits_per_sample.tag   != 0x0102)           return false;
    if (hp->description.tag       != 0x010E)           return false;
    if (hp->samples_per_pixel.tag != 0x0115)           return false;
    if (hp->sample_format.tag     != 0x0153)           return false;
    if (hp->page_index.tag        != SCM_PAGE_INDEX)   return false;
    if (hp->page_offset.tag       != SCM_PAGE_OFFSET)  return false;
    if (hp->page_minimum.tag      != SCM_PAGE_MINIMUM) return false;
    if (hp->page_maximum.tag      != SCM_PAGE_MAXIMUM) return false;
    return true;
}

bool is_ifd(ifd *ip)
{
    if (ip->count                 != SCM_IFD_COUNT)    return false;
    if (ip->image_width.tag       != 0x0100)           return false;
    if (ip->image_length.tag      != 0x0101)           return false;
    if (ip->bits_per_sample.tag   != 0x0102)           return false;
    if (ip->compression.tag       != 0x0103)           return false;
    if (ip->interpretation.tag    != 0x0106)           return false;
    if (ip->strip_offsets.tag     != 0x0111)           return false;
    if (ip->orientation.tag       != 0x0112)           return false;
    if (ip->samples_per_pixel.tag != 0x0115)           return false;
    if (ip->rows_per_strip.tag    != 0x0116)           return false;
    if (ip->strip_byte_counts.tag != 0x0117)           return false;
    if (ip->configuration.tag     != 0x011C)           return false;
    if (ip->predictor.tag         != 0x013D)           return false;
    if (ip->sample_format.tag     != 0x0153)           return false;
    return true;
}

//------------------------------------------------------------------------------

// Return the size in bytes of a datum of the given TIFF type;

size_t tifsizeof(uint16_t t)
{
    switch (t)
    {
        case  1: return 1;  // BYTE
        case  2: return 1;  // ASCII
        case  3: return 2;  // SHORT
        case  4: return 4;  // LONG
        case  5: return 8;  // RATIONAL
        case  6: return 1;  // SBYTE
        case  7: return 1;  // UNDEFINED
        case  8: return 2;  // SSHORT
        case  9: return 4;  // SLONG
        case 10: return 8;  // SRATIONAL
        case 11: return 4;  // FLOAT
        case 12: return 8;  // DOUBLE
        case 16: return 8;  // LONG8
        case 17: return 8;  // SLONG8
        case 18: return 8;  // IFD8
    }
    return 0;
}

// Determine and return the TIFF photometric interpretation.

uint64_t scm_pint(scm *s)
{
    if (s->c == 1) return 1;     // Bilevel black-is-zero
    else           return 3;     // RGB
}

// Determine and return the TIFF format of each sample.

uint16_t scm_form(scm *s)
{
    if (s->b == 32) return 3;     // IEEE floating point
    if (s->g)       return 2;     // Signed integer data
    else            return 1;     // Unsigned integer data
}

// Determine and return the TIFF type of each sample.

uint16_t scm_type(scm *s)
{
    if (s->b == 32) return 11;    // FLOAT
    if (s->b == 64) return 12;    // DOUBLE

    if (s->g)
    {
        if (s->b ==  8) return 6; // SBYTE
        if (s->b == 16) return 8; // SSHORT
    }
    else
    {
        if (s->b ==  8) return 1; // BYTE
        if (s->b == 16) return 3; // SHORT
    }
    return 7;                     // UNDEFINED
}

// Choose a horizontal differencing predection algorithm for this data.

uint64_t scm_hdif(scm *s)
{
    if (s->b ==  8) return 2;
    if (s->b == 16) return 2;

    return 1;
}

//------------------------------------------------------------------------------

// Clamp the given value to a signed unit range.

static inline float sclamp(float k)
{
    if      (k < -1.f) return -1.f;
    else if (k >  1.f) return  1.f;
    else               return    k;
}

// Clamp the given value to an unsigned unit range.

static inline float uclamp(float k)
{
    if      (k <  0.f) return  0.f;
    else if (k >  1.f) return  1.f;
    else               return    k;
}

// Encode the n values in floating point buffer f to the raw buffer p with
// b bits per sample and sign s.

void ftob(void *p, const float *f, size_t n, int b, int g)
{
    size_t i;

    if      (b ==  8 && g == 0)
        for (i = 0; i < n; ++i)
            ((unsigned char  *) p)[i] = (unsigned char)  (uclamp(f[i]) * 255);

    else if (b == 16 && g == 0)
        for (i = 0; i < n; ++i)
            ((unsigned short *) p)[i] = (unsigned short) (uclamp(f[i]) * 65535);

    else if (b ==  8 && g == 1)
        for (i = 0; i < n; ++i)
            ((char  *) p)[i] = (char)  (sclamp(f[i]) * 127);

    else if (b == 16 && g == 1)
        for (i = 0; i < n; ++i)
            ((short *) p)[i] = (short) (sclamp(f[i]) * 32767);

    else if (b == 32)
        for (i = 0; i < n; ++i)
            ((float *) p)[i] = (float) (f[i]);
}

// Decode the n values in raw buffer p to the floating point buffer f assuming
// b bits per sample and sign s.

void btof(const void *p, float *f, size_t n, int b, int g)
{
    size_t i;

    if      (b ==  8 && g == 0)
        for (i = 0; i < n; ++i)
            f[i] = ((unsigned char  *) p)[i] / 255.f;

    else if (b == 16 && g == 0)
        for (i = 0; i < n; ++i)
            f[i] = ((unsigned short *) p)[i] / 65535.f;

    else if (b ==  8 && g == 1)
        for (i = 0; i < n; ++i)
            f[i] = ((char  *) p)[i] / 127.f;

    else if (b == 16 && g == 1)
        for (i = 0; i < n; ++i)
            f[i] = ((short *) p)[i] / 32767.f;

    else if (b == 32)
        for (i = 0; i < n; ++i)
            f[i] = ((float *) p)[i];
}

// Encode the given buffer using the horizontal differencing predictor.

void enhdif(void *p, int n, int c, int b)
{
    const int s = n * c;
    const int m = n - 1;
    const int i = 0;

    if      (b == 8)
    {
        char  *q = (char  *) p;
//      for         (int i = 0; i < n; ++i)
            for     (int j = m; j > 0; --j)
                for (int k = 0; k < c; ++k)
                    q[i * s + j * c + k] -= q[i * s + (j - 1) * c + k];
    }
    else if (b == 16)
    {
        short *q = (short *) p;
//      for         (int i = 0; i < n; ++i)
            for     (int j = m; j > 0; --j)
                for (int k = 0; k < c; ++k)
                    q[i * s + j * c + k] -= q[i * s + (j - 1) * c + k];
    }
    else if (b == 32)
    {
//      float *q = (float *) p;
//      for         (int i = 0; i < n; ++i)
//          for     (int j = m; j > 0; --j)
//              for (int k = 0; k < c; ++k)
//                  q[i * s + j * c + k] -= q[i * s + (j - 1) * c + k];
    }
}

// Decode the given buffer using the horizontal differencing predictor.

void dehdif(void *p, int n, int c, int b)
{
    const int s = n * c;
    const int m = n - 1;
    const int i = 0;

    if      (b == 8)
    {
        char  *q = (char  *) p;
//      for         (int i = 0; i < n; ++i)
            for     (int j = 0; j < m; ++j)
                for (int k = 0; k < c; ++k)
                    q[i * s + (j+1) * c + k] += q[i * s + j * c + k];
    }
    else if (b == 16)
    {
        short *q = (short *) p;
//      for         (int i = 0; i < n; ++i)
            for     (int j = 0; j < m; ++j)
                for (int k = 0; k < c; ++k)
                    q[i * s + (j+1) * c + k] += q[i * s + j * c + k];
    }
    else if (b == 32)
    {
//      float *q = (float *) p;
//      for         (int i = 0; i < n; ++i)
//          for     (int j = 0; j < m; ++j)
//              for (int k = 0; k < c; ++k)
//                  q[i * s + (j+1) * c + k] += q[i * s + j * c + k];
    }
}

//------------------------------------------------------------------------------
// The following ancillary functions perform the fine-grained tasks of binary
// data conversion, compression and decompression, and application of the
// horizontal difference predictor. These are abstracted into tiny adapter
// functions to ease the implementation of threaded file I/O with OpenMP.

// Translate rows i through i+r from floating point to binary and back.

void tobin(scm *s, uint8_t *bin, const float *dat, int i)
{
    const int n = s->n + 2;
    const int m = s->c * n;
    const int d = s->b * m / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        ftob(bin + j * d, dat + (i + j) * m, (size_t) m, s->b, s->g);
}

void frombin(scm *s, const uint8_t *bin, float *dat, int i)
{
    const int n = s->n + 2;
    const int m = s->c * n;
    const int d = s->b * m / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        btof(bin + j * d, dat + (i + j) * m, (size_t) m, s->b, s->g);
}

// Apply or reverse the horizontal difference predector in rows i through i+r.

void todif(scm *s, uint8_t *bin, int i)
{
    const int n = s->n + 2;
    const int d = s->c * s->b * n / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        enhdif(bin + j * d, n, s->c, s->b);
}

void fromdif(scm *s, uint8_t *bin, int i)
{
    const int n = s->n + 2;
    const int d = s->c * s->b * n / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        dehdif(bin + j * d, n, s->c, s->b);
}

// Compress or decompress rows i through i+r.

void tozip(scm *s, uint8_t *bin, int i, uint8_t *zip, uint32_t *c)
{
    uLong l = (uLong) ((s->n + 2) * min(s->r, s->n + 2 - i) * s->c * s->b / 8);
    uLong z = compressBound(l);

    compress((Bytef *) zip, &z, (const Bytef *) bin, l);
    *c = (uint32_t) z;
}

void fromzip(scm *s, uint8_t *bin, int i, uint8_t *zip, uint32_t c)
{
    uLong l = (uLong) ((s->n + 2) * min(s->r, s->n + 2 - i) * s->c * s->b / 8);
    uLong z = (uLong) c;

    uncompress((Bytef *) bin, &l, (const Bytef *) zip, z);
}

//------------------------------------------------------------------------------


