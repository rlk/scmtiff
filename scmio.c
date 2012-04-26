// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>

#include "scmdat.h"
#include "scmio.h"
#include "util.h"
#include "err.h"

//------------------------------------------------------------------------------

// Allocate properly-sized bin and zip scratch buffers for SCM s.

int scm_alloc(scm *s)
{
    size_t bs = (size_t) s->r * (size_t) (s->n + 2)
              * (size_t) s->c * (size_t)  s->b / 8;
    size_t zs = compressBound(bs);

    size_t c = (size_t) (s->n + 2 + s->r - 1) / (size_t) s->r;

    if ((s->binv = (uint8_t **) calloc(c, sizeof (uint8_t *))) &&
        (s->zipv = (uint8_t **) calloc(c, sizeof (uint8_t *))))
    {
        for (size_t i = 0; i < c; i++)
        {
            s->binv[i] = (uint8_t *) malloc(bs);
            s->zipv[i] = (uint8_t *) malloc(zs);
        }
        return 1;
    }
    return 0;
}

// Free the bin and zip scratch buffers.

void scm_free(scm *s)
{
    if (s->r)
    {
        int c = (s->n + 2 + s->r - 1) / s->r;

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

// Read from the SCM file at the given offset, to the given buffer.

int scm_read(scm *s, void *ptr, size_t len, long long o)
{
    if (fseeko(s->fp, o, SEEK_SET) == 0)
    {
        if (fread(ptr, 1, len, s->fp) == len)
        {
            return 1;
        }
        else syserr("Failed to read SCM");
    }
    else syserr("Failed to seek SCM");

    return -1;
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

int scm_read_header(scm *s, header *h)
{
    assert(s);
    assert(h);

    if (fseeko(s->fp, 0, SEEK_SET) == 0)
    {
        if (fread(h, sizeof (header), 1, s->fp) == 1)
        {
            if (is_header(h))
            {
                return 1;
            }
            else apperr("File is not an SCM TIFF");
        }
        else syserr("Failed to read SCM header");
    }
    else syserr("Failed to seek SCM");

    return -1;
}

// Write the SCM TIFF header.

int scm_write_header(scm *s, header *h)
{
    assert(s);
    assert(h);

    if (fseeko(s->fp, 0, SEEK_SET) == 0)
    {
        if (fwrite(h, sizeof (header), 1, s->fp) == 1)
        {
            return 1;
        }
        else syserr("Failed to write SCM header");
    }
    else syserr("Failed to seek SCM");

    return -1;
}

//------------------------------------------------------------------------------

// Initialize an SCM TIFF field.

void scm_set_field(field *fp, uint16_t t, uint16_t y, uint64_t c, uint64_t o)
{
    fp->tag    = t;
    fp->type   = y;
    fp->count  = c;
    fp->offset = o;
}

// Read the value of an IFD field. The length of buffer p is defined by the type
// and count of the field f. If the size of the value exceeds the offset size,
// read the value from the file. The file position should not be trusted upon
// return.

int scm_read_field(scm *s, field *f, void *p)
{
    size_t n = f->count * tifsizeof(f->type);

    if (n <= sizeof (f->offset))
    {
        memcpy(p, &f->offset, n);
        return 1;
    }
    else
    {
        if (fseeko(s->fp, (long long) f->offset, SEEK_SET) == 0)
        {
            if (fread(p, 1, n, s->fp) == n)
            {
                return 1;
            }
            else syserr("Failed to read field offset");
        }
        else syserr("Failed to seek field offset");
    }
    return -1;
}

// Write the given value p to field f. The length of buffer p is defined by the
// type and count of the field f. If this size exceeds the offset size, write
// the value to the file. CAVEAT: If the offset is zero, write the value at the
// current file pointer. If the offset is non-zero, then assume we're updating a
// prior value, and write to the file at that offset. The file pointer should
// not be trusted upon return.

int scm_write_field(scm *s, field *f, const void *p)
{
    size_t n = f->count * tifsizeof(f->type);
    long long o;

    if (n <= sizeof (f->offset))
    {
        memcpy(&f->offset, p, n);
        return 1;
    }
    else
    {
        if (f->offset)
        {
            if (fseeko(s->fp, (long long) f->offset, SEEK_SET) == -1)
            {
                if (fwrite(p, 1, n, s->fp) == n)
                {
                    return 1;
                }
                else syserr("Failed to read field offset");
            }
            else syserr("Failed to seek field offset");
        }
        else
        {
            if ((o = scm_write(s, p, n)) > 0)
            {
                f->offset = (uint64_t) o;
                return 1;
            }
        }
    }
    return -1;
}

//------------------------------------------------------------------------------

// Initialize an HFD with defaults for SCM s.

int scm_init_hfd(scm *s, hfd *i)
{
    if ((size_t) s->c * tifsizeof(scm_type(s)) <= sizeof (uint64_t))
    {
        uint16_t f = (uint16_t) scm_form(s);
        uint16_t b = (uint16_t) s->b;
        uint64_t c = (uint64_t) s->c;

        scm_set_field(&i->image_width,       0x0100, 3, 1, (uint64_t) s->n + 2);
        scm_set_field(&i->image_length,      0x0101, 3, 1, (uint64_t) s->n + 2);
        scm_set_field(&i->samples_per_pixel, 0x0115, 3, 1, (uint64_t) s->c);
        scm_set_field(&i->bits_per_sample,   0x0102, 3, c, 0);
        scm_set_field(&i->sample_format,     0x0153, 3, c, 0);
        scm_set_field(&i->description,       0x0116, 0, 0, 0);
        scm_set_field(&i->rows_per_strip,    0x0116, 0, 0, 0);

        for (int k = 0; k < 4; ++k)
        {
            ((uint16_t *) &i->bits_per_sample.offset)[k] = (k < s->c) ? b : 0;
            ((uint16_t *) &i->sample_format  .offset)[k] = (k < s->c) ? f : 0;
        }

        i->count = SCM_HFD_COUNT;
        i->next  = 0;

        return 1;
    }
    else apperr("Unsupported pixel configuration (more than 64 BPP)");

    return -1;
}

// Read an IFD at offset o of SCM TIFF s.

int scm_read_hfd(scm *s, hfd *i, long long o)
{
    assert(s);
    assert(i);

    if (o)
    {
        if (fseeko(s->fp, o, SEEK_SET) == 0)
        {
            if (fread(i, sizeof (hfd), 1, s->fp) == 1)
            {
                if (is_hfd(i))
                {
                    return 1;
                }
                else apperr("File is not an SCM TIFF");
            }
            else syserr("Failed to read SCM HFD");
        }
        else syserr("Failed to seek SCM");
    }
    return -1;
}

// Write an HFD at offset o of SCM TIFF s.

int scm_write_hfd(scm *s, hfd *i, long long o)
{
    assert(s);
    assert(i);

    if (fseeko(s->fp, o, SEEK_SET) == 0)
    {
        if (fwrite(i, sizeof (hfd), 1, s->fp) == 1)
        {
            return 1;
        }
        else syserr("Failed to write SCM HFD");
    }
    else syserr("Failed to seek SCM");

    return -1;
}

//------------------------------------------------------------------------------

// Initialize an IFD with defaults for SCM s.

int scm_init_ifd(scm *s, ifd *i)
{
    if ((size_t) s->c * tifsizeof(scm_type(s)) <= sizeof (uint64_t))
    {
        uint16_t f = (uint16_t) scm_form(s);
        uint16_t b = (uint16_t) s->b;
        uint64_t c = (uint64_t) s->c;

        scm_set_field(&i->image_width,       0x0100, 3, 1, (uint64_t) s->n + 2);
        scm_set_field(&i->image_length,      0x0101, 3, 1, (uint64_t) s->n + 2);
        scm_set_field(&i->samples_per_pixel, 0x0115, 3, 1, (uint64_t) s->c);
        scm_set_field(&i->rows_per_strip,    0x0116, 3, 1, (uint64_t) s->r);
        scm_set_field(&i->interpretation,    0x0106, 3, 1, scm_pint(s));
        scm_set_field(&i->predictor,         0x013D, 3, 1, scm_hdif(s));
        scm_set_field(&i->compression,       0x0103, 3, 1, 8);
        scm_set_field(&i->orientation,       0x0112, 3, 1, 2);
        scm_set_field(&i->configuration,     0x011C, 3, 1, 1);
        scm_set_field(&i->bits_per_sample,   0x0102, 3, c, 0);
        scm_set_field(&i->sample_format,     0x0153, 3, c, 0);
        scm_set_field(&i->strip_offsets,     0x0111, 0, 0, 0);
        scm_set_field(&i->rows_per_strip,    0x0116, 0, 0, 0);
        scm_set_field(&i->strip_byte_counts, 0x0117, 0, 0, 0);
        scm_set_field(&i->page_number,       0x0129, 0, 0, 0);

        for (int k = 0; k < 4; ++k)
        {
            ((uint16_t *) &i->bits_per_sample.offset)[k] = (k < s->c) ? b : 0;
            ((uint16_t *) &i->sample_format  .offset)[k] = (k < s->c) ? f : 0;
        }

        i->count = SCM_IFD_COUNT;
        i->next  = 0;

        return 1;
    }
    else apperr("Unsupported pixel configuration (more than 64 BPP)");

    return -1;
}

// Read an IFD at offset o of SCM TIFF s.

int scm_read_ifd(scm *s, ifd *i, long long o)
{
    assert(s);
    assert(i);

    if (o)
    {
        if (fseeko(s->fp, o, SEEK_SET) == 0)
        {
            if (fread(i, sizeof (ifd), 1, s->fp) == 1)
            {
                if (is_ifd(i))
                {
                    return 1;
                }
                else apperr("File is not an SCM TIFF");
            }
            else syserr("Failed to read SCM IFD");
        }
        else syserr("Failed to seek SCM");
    }
    return -1;
}

// Write an IFD at offset o of SCM TIFF s.

int scm_write_ifd(scm *s, ifd *i, long long o)
{
    assert(s);
    assert(i);

    if (fseeko(s->fp, o, SEEK_SET) == 0)
    {
        if (fwrite(i, sizeof (ifd), 1, s->fp) == 1)
        {
            return 1;
        }
        else syserr("Failed to write SCM IFD");
    }
    else syserr("Failed to seek SCM");

    return -1;
}

//------------------------------------------------------------------------------

// Read the first IFD and determine basic image parameters from it, including
// the image size, channel count, bits-per-sample, and sample format.

int scm_read_preable(scm *s)
{
    header h;
    hfd    d;

    if (scm_read_header(s, &h) == 1)
    {
        if (scm_read_ifd(s, &s->D, (long long) h.first_ifd) == 1)
        {
            if (scm_read_header(s, &h) == 1)
            {
                if (scm_read_hfd(s, &d, h.first_ifd) == 1)
                {
                    s->n = (int)               d.image_width      .offset - 2;
                    s->c = (int)               d.samples_per_pixel.offset;
                    s->r = (int)               d.rows_per_strip   .offset;
                    s->b = (int) ((uint16_t *) d.bits_per_sample  .offset)[0];
                    s->g = (2 == ((uint16_t *) d.sample_format    .offset)[0]);

                    return 1;
                }
            }
        }
    }
    return -1;
}

int scm_write_preamble(scm *s, const char *str)
{
}

#if 0
// Read the first IFD and determine basic image parameters from it, including
// the image size, channel count, bits-per-sample, and sample format.

int scm_read_preface(scm *s)
{
    header h;

    if (scm_read_header(s, &h) == 1)
    {
        if (scm_read_ifd(s, &s->D, (long long) h.first_ifd) == 1)
        {
        	// Image size and channel count.

            s->n = (int) s->D.image_width.offset - 2;
            s->c = (int) s->D.samples_per_pixel.offset;
            s->r = (int) s->D.rows_per_strip.offset;

            // Image description string.

            if ((s->str = (char *) malloc(s->D.description.count)))
                scm_read_field(s, &s->D.description, s->str);

            // Bits-per-sample and sample format.

        	uint16_t bv[s->c];
        	uint16_t fv[s->c];

            if (scm_read_field(s, &s->D.bits_per_sample, bv) == 1)
	            s->b = (bv[0]);
            if (scm_read_field(s, &s->D.sample_format,   fv) == 1)
	            s->g = (fv[0] == 2) ? 1 : 0;

            return 1;
        }
    }
    return -1;
}

// Initialize the template IFD with globally uniform values. This may involve
// file writes for any values that exceed the size of the field offset.

int scm_write_preface(scm *s, const char *str)
{
    header h;

    set_header(&h);

    if (scm_write_header(s, &h) == 1)
    {
        uint16_t l = (uint16_t) strlen(str) + 1;
        uint64_t c = (uint64_t) s->c;
        uint64_t i = scm_pint(s);
        uint64_t p = scm_hdif(s);

        uint16_t bv[c];
        uint16_t fv[c];

        for (int k = 0; k < s->c; ++k)
        {
            bv[k] = (uint16_t) s->b;
            fv[k] = scm_form(s);
        }

        set_field(&s->D.subfile_type,      0x00FE, 4, 1, 2);
        set_field(&s->D.image_width,       0x0100, 3, 1, (uint64_t) (s->n + 2));
        set_field(&s->D.image_length,      0x0101, 3, 1, (uint64_t) (s->n + 2));
        set_field(&s->D.bits_per_sample,   0x0102, 3, c, 0);
        set_field(&s->D.compression,       0x0103, 3, 1, 8);
        set_field(&s->D.interpretation,    0x0106, 3, 1, i);
        set_field(&s->D.description,       0x010E, 2, l, 0);
        set_field(&s->D.orientation,       0x0112, 3, 1, 2);
        set_field(&s->D.samples_per_pixel, 0x0115, 3, 1, (uint64_t) s->c);
        set_field(&s->D.rows_per_strip,    0x0116, 3, 1, (uint64_t) s->r);
        set_field(&s->D.configuration,     0x011C, 3, 1, 1);
        set_field(&s->D.predictor,         0x013D, 3, 1, p);
        set_field(&s->D.sample_format,     0x0153, 3, c, 0);
        set_field(&s->D.page_catalog,      0xFFB1, 4, 0, 0);
        set_field(&s->D.page_minima,       0xFFB2, 4, 0, 0);
        set_field(&s->D.page_maxima,       0xFFB3, 4, 0, 0);

        scm_write_field(s, &s->D.description,    str);
        scm_align(s);
        scm_write_field(s, &s->D.bits_per_sample, bv);
        scm_align(s);
        scm_write_field(s, &s->D.sample_format,   fv);
        scm_align(s);

        s->D.count = SCM_FIELD_COUNT;

        return 1;
    }
    return -1;
}
#endif


//------------------------------------------------------------------------------

// Read a page of data into the zip caches. Store the strip offsets and lengths
// in the given arrays. This is the serial part of the parallel input handler.

int scm_read_zips(scm *s, uint8_t **zv,
                          uint64_t  oo,
                          uint64_t  lo,
                          uint16_t  sc, uint64_t *o, uint32_t *l)
{
    // Read the strip offset and length arrays.

    if (scm_read(s, o, sc * sizeof (uint64_t), (long long) oo) == -1) return -1;
    if (scm_read(s, l, sc * sizeof (uint32_t), (long long) lo) == -1) return -1;

    // Read each strip.

    for (int i = 0; i < sc; i++)
        if (scm_read(s, zv[i], (size_t) l[i], (long long) o[i]) == -1)
            return -1;

    return 1;
}

// Write a page of data from the zip caches. This is the serial part of the
// parallel output handler. Also, in concert with scm_read_zips, this function
// allows data to be copied from one SCM to another without the computational
// cost of an unnecessary encode-decode cycle.

int scm_write_zips(scm *s, uint8_t **zv,
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
            return -1;

    // Write the strip offset and length arrays.

    if ((t = scm_write(s, o, c * sizeof (uint64_t))) > 0)
        *oo = (uint64_t) t;
    else
        return -1;

    if ((t = scm_write(s, l, c * sizeof (uint32_t))) > 0)
        *lo = (uint64_t) t;
    else
        return -1;

    return 1;
}

//------------------------------------------------------------------------------

// Read and decode a page of data to the given float buffer.

int scm_read_data(scm *s, float *p, uint64_t oo,
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
        return 1;
    }
    return -1;
}

// Encode and write a page of data from the given float buffer.

int scm_write_data(scm *s, const float *p, uint64_t *oo,
                                           uint64_t *lo,
                                           uint16_t *sc)
{
    // Strip count is total rows / rows-per-strip rounded up.

    int i, c = (s->n + 2 + s->r - 1) / s->r;
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
// IFD linked-to by the header.

int scm_link_list(scm *s, long long c, long long p)
{
    header h;
    ifd    i;

    if (p)
    {
        if (scm_read_ifd(s, &i, p) == 1)
        {
            i.next = (uint64_t) c;

            if (scm_write_ifd(s, &i, p) == 1)
            {
                return 1;
            }
            else apperr("Failed to write previous IFD");
        }
        else apperr("Failed to read previous IFD");
   }
    else
    {
        if (scm_read_header(s, &h) == 1)
        {
            h.first_ifd = (uint64_t) c;

            if (scm_write_header(s, &h) == 1)
            {
                return 1;
            }
            else apperr("Failed to write header");
        }
        else apperr("Failed to write header");
    }
    return -1;
}

//------------------------------------------------------------------------------
