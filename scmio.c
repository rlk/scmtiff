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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <zlib.h>

#include "scmdat.h"
#include "scmio.h"
#include "util.h"
#include "err.h"

//------------------------------------------------------------------------------

// Allocate properly-sized bin and zip scratch buffers for SCM s.

bool scm_alloc(scm *s)
{
    size_t bs = (size_t) s->r * (size_t) s->n
              * (size_t) s->c * (size_t) s->b / 8;
    size_t zs = compressBound(bs);

    size_t c = (size_t) (s->n + s->r - 1) / (size_t) s->r;

    if ((s->binv = (uint8_t **) calloc(c, sizeof (uint8_t *))) &&
        (s->zipv = (uint8_t **) calloc(c, sizeof (uint8_t *))))
    {
        for (size_t i = 0; i < c; i++)
        {
            s->binv[i] = (uint8_t *) malloc(bs);
            s->zipv[i] = (uint8_t *) malloc(zs);
        }
        return true;
    }
    return false;
}

// Free the bin and zip scratch buffers.

void scm_free(scm *s)
{
    if (s->r)
    {
        int c = (s->n + s->r - 1) / s->r;

        for (int i = 0; i < c; i++)
        {
            if (s->zipv) free(s->zipv[i]);
            if (s->binv) free(s->binv[i]);
        }
    }
    free(s->zipv);
    free(s->binv);

    s->zipv = NULL;
    s->binv = NULL;
}

//------------------------------------------------------------------------------

// Move the SCM file pointer to the end of the file

bool scm_ffwd(scm *s)
{
    if (fseeko(s->fp, 0, SEEK_END) == 0)
    {
        return true;
    }
    else syserr("Failed to seek SCM");

    return false;
}

// Move the SCM file pointer to offset o.

bool scm_seek(scm *s, long long o)
{
    if (fseeko(s->fp, o, SEEK_SET) == 0)
    {
        return true;
    }
    else syserr("Failed to seek SCM");

    return false;
}

// Read from the SCM file at the given offset, to the given buffer.

bool scm_read(scm *s, void *ptr, size_t len, long long o)
{
    if (scm_seek(s, o) >= 0)
    {
        if (fread(ptr, 1, len, s->fp) == len)
        {
            return true;
        }
        else syserr("Failed to read SCM");
    }
    else syserr("Failed to seek SCM");

    return false;
}

// Write the given buffer to the SCM file, returning the offset of the beginning
// of the write.

long long scm_write(scm *s, const void *ptr, size_t len)
{
    long long o;

    if ((o = ftello(s->fp)) >= 0)
    {
        if (fwrite(ptr, 1, len, s->fp) == len)
        {
            return o;
        }
        else syserr("Failed to write SCM");
    }
    else syserr("Failed to tell SCM");

    return -1;
}

// Ensure that the current SCM TIFF position falls on a TIFF word boundary by
// writing a single byte if the current file offset is odd.

long long scm_align(scm *s)
{
    char  c = 0;
    long long o;

    if ((o = ftello(s->fp)) >= 0)
    {
        if ((o & 1))
        {
            if (fwrite(&c, 1, 1, s->fp) == 1)
            {
                return o + 1;
            }
            else syserr("Failed to align SCM");
        }
        return o;
    }
    else syserr("Failed to tell SCM");

    return -1;
}

//------------------------------------------------------------------------------

// Initialize an SCM TIFF field.

void scm_field(field *fp, uint16_t t, uint16_t y, uint64_t c, uint64_t o)
{
    fp->tag    = t;
    fp->type   = y;
    fp->count  = c;
    fp->offset = o;
}

//------------------------------------------------------------------------------

// Initialize an SCM TIFF header.

void scm_init_header(header *h)
{
    h->endianness = 0x4949;
    h->version    = 0x002B;
    h->offsetsize = 8;
    h->zero       = 0;
    h->first_ifd  = 0;
}

/// Read the SCM TIFF header.

bool scm_read_header(scm *s, header *h)
{
    assert(s);
    assert(h);

    if (scm_read(s, h, sizeof (header), 0) >= 0)
    {
        if (is_header(h))
        {
            return true;
        }
        else apperr("%s is not an SCM TIFF", s->name);
    }
    return false;
}

// Write the SCM TIFF header.

long long scm_write_header(scm *s, header *h)
{
    assert(s);
    assert(h);

    if (scm_seek(s, 0))
    {
        return scm_write(s, h, sizeof (header));
    }
    return -1;
}

//------------------------------------------------------------------------------

// Initialize an HFD with defaults for SCM s.

bool scm_init_hfd(scm *s, hfd *d)
{
    if ((size_t) s->c * tifsizeof(scm_type(s)) <= sizeof (uint64_t))
    {
        uint16_t f = (uint16_t) scm_form(s);
        uint16_t b = (uint16_t) s->b;
        uint64_t c = (uint64_t) s->c;

        scm_field(&d->image_width,       0x0100,  3, 1, (uint64_t) s->n);
        scm_field(&d->image_length,      0x0101,  3, 1, (uint64_t) s->n);
        scm_field(&d->samples_per_pixel, 0x0115,  3, 1, (uint64_t) s->c);
        scm_field(&d->rows_per_strip,    0x0116,  3, 1, (uint64_t) s->r);
        scm_field(&d->bits_per_sample,   0x0102,  3, c, 0);
        scm_field(&d->strip_offsets,     0x0111, 16, 0, 0);
        scm_field(&d->strip_byte_counts, 0x0117,  4, 0, 0);
        scm_field(&d->sample_format,     0x0153,  3, c, 0);
        scm_field(&d->description,       0x010E,  2, 0, 0);

        scm_field(&d->page_index,   SCM_PAGE_INDEX,   0, 0, 0);
        scm_field(&d->page_offset,  SCM_PAGE_OFFSET,  0, 0, 0);
        scm_field(&d->page_minimum, SCM_PAGE_MINIMUM, 0, 0, 0);
        scm_field(&d->page_maximum, SCM_PAGE_MAXIMUM, 0, 0, 0);

        for (int k = 0; k < 4; ++k)
        {
            ((uint16_t *) &d->bits_per_sample.offset)[k] = (k < s->c) ? b : 0;
            ((uint16_t *) &d->sample_format  .offset)[k] = (k < s->c) ? f : 0;
        }

        d->count = SCM_HFD_COUNT;
        d->next  = 0;

        return true;
    }
    else apperr("%s: Unsupported pixel configuration (> 64 BPP)", s->name);

    return false;
}

// Read an IFD at offset o of SCM TIFF s.

bool scm_read_hfd(scm *s, hfd *d, long long o)
{
    assert(s);
    assert(d);

    if (o && scm_read(s, d, sizeof (hfd), o) >= 0)
    {
        if (is_hfd(d))
        {
            return true;
        }
        else apperr("%s is not an SCM TIFF", s->name);
    }
    return false;
}

// Write an HFD after the header and return its offset.

long long scm_write_hfd(scm *s, hfd *d, long long o)
{
    assert(s);
    assert(d);

    if (o)
    {
        if (scm_seek(s, o) >= 0)
        {
            return scm_write(s, d, sizeof (hfd));
        }
        else return -1;
    }
    else return scm_write(s, d, sizeof (hfd));
}

//------------------------------------------------------------------------------

// Initialize an IFD with defaults for SCM s.

bool scm_init_ifd(scm *s, ifd *d)
{
    if ((size_t) s->c * tifsizeof(scm_type(s)) <= sizeof (uint64_t))
    {
        uint16_t f = (uint16_t) scm_form(s);
        uint16_t b = (uint16_t) s->b;
        uint64_t c = (uint64_t) s->c;

        scm_field(&d->image_width,       0x0100, 3, 1, (uint64_t) s->n);
        scm_field(&d->image_length,      0x0101, 3, 1, (uint64_t) s->n);
        scm_field(&d->samples_per_pixel, 0x0115, 3, 1, (uint64_t) s->c);
        scm_field(&d->rows_per_strip,    0x0116, 3, 1, (uint64_t) s->r);
        scm_field(&d->interpretation,    0x0106, 3, 1, scm_pint(s));
        scm_field(&d->predictor,         0x013D, 3, 1, scm_hdif(s));
        scm_field(&d->compression,       0x0103, 3, 1, 8);
        scm_field(&d->orientation,       0x0112, 3, 1, 2);
        scm_field(&d->configuration,     0x011C, 3, 1, 1);
        scm_field(&d->bits_per_sample,   0x0102, 3, c, 0);
        scm_field(&d->sample_format,     0x0153, 3, c, 0);
        scm_field(&d->strip_offsets,     0x0111, 0, 0, 0);
        scm_field(&d->strip_byte_counts, 0x0117, 0, 0, 0);
        scm_field(&d->page_number,       0x0129, 0, 0, 0);

        for (int k = 0; k < 4; ++k)
        {
            ((uint16_t *) &d->bits_per_sample.offset)[k] = (k < s->c) ? b : 0;
            ((uint16_t *) &d->sample_format  .offset)[k] = (k < s->c) ? f : 0;
        }

        d->count = SCM_IFD_COUNT;
        d->next  = 0;

        return true;
    }
    else apperr("%s: Unsupported pixel configuration (> 64 BPP)", s->name);

    return false;
}

// Read an IFD at offset o of SCM TIFF s.

bool scm_read_ifd(scm *s, ifd *d, long long o)
{
    assert(s);
    assert(d);

    if (o && scm_read(s, d, sizeof (ifd), o) >= 0)
    {
        if (is_ifd(d))
        {
            return true;
        }
        else apperr("%s is not an SCM TIFF", s->name);
    }
    return false;
}

// If o is non-zero, write an IFD at offset o of SCM TIFF s. Otherwise, write
// the IFD at the current file position and return the offset.

long long scm_write_ifd(scm *s, ifd *d, long long o)
{
    assert(s);
    assert(d);

    if (o)
    {
        if (scm_seek(s, o) >= 0)
        {
            return scm_write(s, d, sizeof (ifd));
        }
        else return -1;
    }
    else return scm_write(s, d, sizeof (ifd));
}

//------------------------------------------------------------------------------

// Read the header and the HFD and determine basic image parameters from it:
// the image size, channel count, bits-per-sample, and sample format.

bool scm_read_preamble(scm *s)
{
    header h;
    hfd    d;

    if (scm_read_header(s, &h))
    {
        if (scm_read_hfd(s, &d, h.first_ifd))
        {
            s->n = (int)                 d.image_width      .offset;
            s->c = (int)                 d.samples_per_pixel.offset;
            s->r = (int)                 d.rows_per_strip   .offset;
            s->b = (int) ((uint16_t *) (&d.bits_per_sample  .offset))[0];
            s->g = (2 == ((uint16_t *) (&d.sample_format    .offset))[0]);

            return true;
        }
    }
    return false;
}

// Write the header and the HFD using the parameters of SCM s.

bool scm_write_preamble(scm *s)
{
    header h;
    hfd    d;

    scm_init_header(&h);

    if (scm_init_hfd(s, &d))
    {
        if (scm_write_header(s, &h) >= 0)
        {
            if ((h.first_ifd = (uint64_t) scm_write_hfd(s, &d, 0)))
            {
                if (scm_write_header(s, &h) >= 0)
                {
                    return true;
                }
            }
        }
    }
    return false;
}


//------------------------------------------------------------------------------

// Read a page of data into the zip caches. Store the strip offsets and lengths
// in the given arrays. This is the serial part of the parallel input handler.

bool scm_read_zips(scm *s, uint8_t **zv,
                           uint64_t  oo,
                           uint64_t  lo,
                           uint16_t  sc, uint64_t *o, uint32_t *l)
{
    // Read the strip offset and length arrays.

    if (!scm_read(s, o, sc * sizeof (uint64_t), (long long) oo)) return false;
    if (!scm_read(s, l, sc * sizeof (uint32_t), (long long) lo)) return false;

    // Read each strip.

    for (int i = 0; i < sc; i++)
        if (!scm_read(s, zv[i], (size_t) l[i], (long long) o[i]))
            return false;

    return true;
}

// Write a page of data from the zip caches. This is the serial part of the
// parallel output handler. Also, in concert with scm_read_zips, this function
// allows data to be copied from one SCM to another without the computational
// cost of an unnecessary encode-decode cycle.

bool scm_write_zips(scm *s, uint8_t **zv,
                            uint64_t *oo,
                            uint64_t *lo,
                            uint16_t *sc, uint64_t *o, uint32_t *l)
{
    size_t c = (size_t) (*sc);
    long long t;

    // Write each strip to the file, noting all offsets.

    for (size_t i = 0; i < c; i++)
        if ((t = scm_write(s, zv[i], (size_t) l[i])) > 0)
            o[i] = (uint64_t) t;
        else
            return false;

    // Write the strip offset and length arrays.

    if ((t = scm_write(s, o, c * sizeof (uint64_t))) > 0)
        *oo = (uint64_t) t;
    else
        return false;

    if ((t = scm_write(s, l, c * sizeof (uint32_t))) > 0)
        *lo = (uint64_t) t;
    else
        return false;

    return true;
}

//------------------------------------------------------------------------------

// Read and decode a page of data to the given float buffer.

bool scm_read_data(scm *s, float *p, uint64_t oo,
                                     uint64_t lo,
                                     uint16_t sc)
{
    // Strip count and rows-per-strip are given by the IFD.

    int i, c = sc;
    uint64_t o[c];
    uint32_t l[c];

    if (scm_read_zips(s, s->zipv, oo, lo, sc, o, l) > 0)
    {
        // Decode each strip.

        #pragma omp parallel for
        for (i = 0; i < c; i++)
        {
            fromzip(s, s->binv[i],    i * s->r, s->zipv[i], l[i]);
            fromdif(s, s->binv[i],    i * s->r);
            frombin(s, s->binv[i], p, i * s->r);
        }
        return true;
    }
    return false;
}

// Encode and write a page of data from the given float buffer.

bool scm_write_data(scm *s, const float *p, uint64_t *oo,
                                            uint64_t *lo,
                                            uint16_t *sc)
{
    // Strip count is total rows / rows-per-strip rounded up.

    int i, c = (s->n + s->r - 1) / s->r;
    uint64_t o[c];
    uint32_t l[c];

    // Encode each strip for writing. This is our hot spot.

    #pragma omp parallel for
    for (i = 0; i < c; i++)
    {
        tobin(s, s->binv[i], p, i * s->r);
        todif(s, s->binv[i],    i * s->r);
        tozip(s, s->binv[i],    i * s->r, s->zipv[i], l + i);
    }

    *sc = (uint16_t) c;

    return scm_write_zips(s, s->zipv, oo, lo, sc, o, l);
}

//------------------------------------------------------------------------------

// Set IFD c to be the "next" of IFD p. If p is zero, set IFD c to be the first
// IFD linked-to by the preamble.

bool scm_link_list(scm *s, long long c, long long p)
{
    if (p)
    {
        ifd i;

        if (scm_read_ifd(s, &i, p))
        {
            i.next = (uint64_t) c;

            if (scm_write_ifd(s, &i, p) > 0)
            {
                return true;
            }
            else apperr("%s: Failed to write previous IFD", s->name);
        }
        else apperr("%s: Failed to read previous IFD", s->name);
   }
    else
    {
        header h;
        hfd    d;

        if (scm_read_header(s, &h))
        {
            if (scm_read_hfd(s, &d, h.first_ifd))
            {
                d.next = (uint64_t) c;

                if (scm_write_hfd(s, &d, h.first_ifd) > 0)
                {
                    return true;
                }
                else apperr("%s: Failed to write preamble", s->name);
            }
            else apperr("%s: Failed to read preamble", s->name);
        }
        else apperr("%s: Failed to read preamble", s->name);
    }
    return false;
}

//------------------------------------------------------------------------------
