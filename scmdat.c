// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdint.h>
#include <stdio.h>

#include "scmdat.h"

//------------------------------------------------------------------------------

// Initialize an SCM TIFF header.

void set_header(header *hp)
{
    hp->endianness = 0x4949;
    hp->version    = 0x002B;
    hp->offsetsize = 8;
    hp->zero       = 0;
    hp->first_ifd  = 0;
}

// Initialize an SCM TIFF field.

void set_field(field *fp, uint16_t t, uint16_t y, uint64_t c, uint64_t o)
{
    fp->tag    = t;
    fp->type   = y;
    fp->count  = c;
    fp->offset = o;
}

//------------------------------------------------------------------------------

int is_header(header *hp)
{
    return (hp->endianness == 0x4949
        &&  hp->version    == 0x002B
        &&  hp->offsetsize == 0x0008
        &&  hp->zero       == 0x0000);
}

int is_ifd(ifd *ip)
{
    return (ip->count == 18

        &&  ip->subfile_type.tag      == 0x00FE
        &&  ip->image_width.tag       == 0x0100
        &&  ip->image_length.tag      == 0x0101
        &&  ip->bits_per_sample.tag   == 0x0102
        &&  ip->compression.tag       == 0x0103
        &&  ip->interpretation.tag    == 0x0106
        &&  ip->description.tag       == 0x010E
        &&  ip->strip_offsets.tag     == 0x0111
        &&  ip->orientation.tag       == 0x0112
        &&  ip->samples_per_pixel.tag == 0x0115
        &&  ip->strip_byte_counts.tag == 0x0117
        &&  ip->configuration.tag     == 0x011C
        &&  ip->predictor.tag         == 0x013D
        &&  ip->sub_ifds.tag          == 0x014A
        &&  ip->sample_format.tag     == 0x0153
        &&  ip->sample_min.tag        == 0x0154
        &&  ip->sample_max.tag        == 0x0155
        &&  ip->page_index.tag        == 0xFFB1);
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
        if (s->b == 8)  return 6; // SBYTE
        if (s->b == 16) return 8; // SSHORT
    }
    else
    {
        if (s->b == 8)  return 1; // BYTE
        if (s->b == 16) return 3; // SHORT
    }
    return 7;                     // UNDEFINED
}

//------------------------------------------------------------------------------

// Encode the n values in floating point buffer f to the raw buffer p with
// b bits per sample and sign s.

void ftob(void *p, const double *f, size_t n, int b, int g)
{
    size_t i;

    if      (b ==  8 && g == 0)
        for (i = 0; i < n; ++i)
            ((unsigned char  *) p)[i] = (unsigned char)  (f[i] * 255);

    else if (b == 16 && g == 0)
        for (i = 0; i < n; ++i)
            ((unsigned short *) p)[i] = (unsigned short) (f[i] * 65535);

    else if (b ==  8 && g == 1)
        for (i = 0; i < n; ++i)
            ((char  *) p)[i] = (char)  (f[i] * 127);

    else if (b == 16 && g == 1)
        for (i = 0; i < n; ++i)
            ((short *) p)[i] = (short) (f[i] * 32767);

    else if (b == 32)
        for (i = 0; i < n; ++i)
            ((float *) p)[i] = (float) (f[i]);
}

// Decode the n values in raw buffer p to the floating point buffer f assuming
// b bits per sample and sign s.

void btof(const void *p, double *f, size_t n, int b, int g)
{
    size_t i;

    if      (b ==  8 && g == 0)
        for (i = 0; i < n; ++i)
            f[i] = ((unsigned char  *) p)[i] / 255.0;

    else if (b == 16 && g == 0)
        for (i = 0; i < n; ++i)
            f[i] = ((unsigned short *) p)[i] / 65535.0;

    else if (b ==  8 && g == 1)
        for (i = 0; i < n; ++i)
            f[i] = ((char  *) p)[i] / 127.0;

    else if (b == 16 && g == 1)
        for (i = 0; i < n; ++i)
            f[i] = ((short *) p)[i] / 32767.0;

    else if (b == 32)
        for (i = 0; i < n; ++i)
            f[i] = (double) ((float *) p)[i];
}

// Encode the given buffer using the horizontal differencing predictor.

void enhdif(void *p, int n, int c, int b)
{
    const int s = n * c;
    const int m = n - 1;

    if      (b == 8)
    {
        char  *q = (char  *) p;
        for         (int i = 0; i < n; ++i)
            for     (int j = m; j > 0; --j)
                for (int k = 0; k < c; ++k)
                    q[i * s + j * c + k] -= q[i * s + (j - 1) * c + k];
    }
    else if (b == 16)
    {
        short *q = (short *) p;
        for         (int i = 0; i < n; ++i)
            for     (int j = m; j > 0; --j)
                for (int k = 0; k < c; ++k)
                    q[i * s + j * c + k] -= q[i * s + (j - 1) * c + k];
    }
    else if (b == 32)
    {
        float *q = (float *) p;
        for         (int i = 0; i < n; ++i)
            for     (int j = m; j > 0; --j)
                for (int k = 0; k < c; ++k)
                    q[i * s + j * c + k] -= q[i * s + (j - 1) * c + k];
    }
}

// Decode the given buffer using the horizontal differencing predictor.

void dehdif(void *p, int n, int c, int b)
{
    const int s = n * c;
    const int m = n - 1;

    if      (b == 8)
    {
        char  *q = (char  *) p;
        for         (int i = 0; i < n; ++i)
            for     (int j = 0; j < m; ++j)
                for (int k = 0; k < c; ++k)
                    q[i * s + (j+1) * c + k] += q[i * s + j * c + k];
    }
    else if (b == 16)
    {
        short *q = (short *) p;
        for         (int i = 0; i < n; ++i)
            for     (int j = 0; j < m; ++j)
                for (int k = 0; k < c; ++k)
                    q[i * s + (j+1) * c + k] += q[i * s + j * c + k];
    }
    else if (b == 32)
    {
        float *q = (float *) p;
        for         (int i = 0; i < n; ++i)
            for     (int j = 0; j < m; ++j)
                for (int k = 0; k < c; ++k)
                    q[i * s + (j+1) * c + k] += q[i * s + j * c + k];
    }
}

//------------------------------------------------------------------------------


