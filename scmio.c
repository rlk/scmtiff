// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>

#include "scmdat.h"
#include "util.h"
#include "err.h"

//------------------------------------------------------------------------------

// Allocate properly-sized bin and zip scratch buffers for SCM s.

int scm_alloc(scm *s)
{
    size_t bs = (s->n + 2) * s->r * s->c * s->b / 8;
    size_t zs = compressBound(bs);

    int c = (s->n + 2 + s->r - 1) / s->r;

    if ((s->binv = (uint8_t **) calloc(c, sizeof (uint8_t *))) &&
        (s->zipv = (uint8_t **) calloc(c, sizeof (uint8_t *))))
    {
        for (int i = 0; i < c; i++)
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

int scm_read(scm *s, void *ptr, size_t len, off_t off)
{
    if (fseeko(s->fp, off, SEEK_SET) == 0)
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

off_t scm_write(scm *s, const void *ptr, size_t len)
{
    off_t o;

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

off_t scm_align(scm *s)
{
    char  c = 0;
    off_t o;

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

// Read the SCM TIFF header.

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
        if (fseeko(s->fp, f->offset, SEEK_SET) == 0)
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
    off_t  o;

    if (n <= sizeof (f->offset))
    {
        memcpy(&f->offset, p, n);
        return 1;
    }
    else
    {
        if (f->offset)
        {
            if (fseeko(s->fp, f->offset, SEEK_SET) == -1)
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

// Read an IFD at offset o of SCM TIFF s.

int scm_read_ifd(scm *s, ifd *i, off_t o)
{
    assert(s);
    assert(i);

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

    return -1;
}

// Write an IFD at offset o of SCM TIFF s.

int scm_write_ifd(scm *s, ifd *i, off_t o)
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

int scm_read_preface(scm *s)
{
    header h;

    if (scm_read_header(s, &h) == 1)
    {
        if (scm_read_ifd(s, &s->D, h.first_ifd) == 1)
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
        uint64_t i = scm_pint(s);
        uint64_t p = scm_hdif(s);
        uint16_t l = strlen(str) + 1;

        uint16_t bv[s->c];
        uint16_t fv[s->c];

        for (int k = 0; k < s->c; ++k)
        {
            bv[k] = s->b;
            fv[k] = scm_form(s);
        }

        set_field(&s->D.subfile_type,      0x00FE, 4, 1,    2);
        set_field(&s->D.image_width,       0x0100, 3, 1,    s->n + 2);
        set_field(&s->D.image_length,      0x0101, 3, 1,    s->n + 2);
        set_field(&s->D.bits_per_sample,   0x0102, 3, s->c, 0);
        set_field(&s->D.compression,       0x0103, 3, 1,    8);
        set_field(&s->D.interpretation,    0x0106, 3, 1,    i);
        set_field(&s->D.description,       0x010E, 2, l,    0);
        set_field(&s->D.orientation,       0x0112, 3, 1,    2);
        set_field(&s->D.samples_per_pixel, 0x0115, 3, 1,    s->c);
        set_field(&s->D.rows_per_strip,    0x0116, 3, 1,    s->r);
        set_field(&s->D.configuration,     0x011C, 3, 1,    1);
        set_field(&s->D.predictor,         0x013D, 3, 1,    p);
        set_field(&s->D.sample_format,     0x0153, 3, s->c, 0);

        scm_write_field(s, &s->D.description,    str);
        scm_align(s);
        scm_write_field(s, &s->D.bits_per_sample, bv);
        scm_align(s);
        scm_write_field(s, &s->D.sample_format,   fv);
        scm_align(s);

        s->D.count = 17;

        return 1;
    }
    return -1;
}

//------------------------------------------------------------------------------
// The following ancillary functions perform the fine-grained tasks of binary
// data conversion, compression and decompression, and application of the
// horizontal difference predictor. These are abstracted into tiny adapter
// functions to ease the implementation of threaded file I/O with OpenMP.

// Translate rows i through i+r from floating point to binary and back.

static void tobin(scm *s, uint8_t *bin, const float *dat, int i)
{
    const int n = s->n + 2;
    const int m = s->c * n;
    const int d = s->b * m / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        ftob(bin + j * d, dat + (i + j) * m, m, s->b, s->g);
}

static void frombin(scm *s, const uint8_t *bin, float *dat, int i)
{
    const int n = s->n + 2;
    const int m = s->c * n;
    const int d = s->b * m / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        btof(bin + j * d, dat + (i + j) * m, m, s->b, s->g);
}

// Apply or reverse the horizontal difference predector in rows i through i+r.

static void todif(scm *s, uint8_t *bin, int i)
{
    const int n = s->n + 2;
    const int d = s->c * s->b * n / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        enhdif(bin + j * d, n, s->c, s->b);
}

static void fromdif(scm *s, uint8_t *bin, int i)
{
    const int n = s->n + 2;
    const int d = s->c * s->b * n / 8;

    for (int j = 0; j < s->r && i + j < n; ++j)
        dehdif(bin + j * d, n, s->c, s->b);
}

// Compress or decompress rows i through i+r.

static void tozip(scm *s, uint8_t *bin, int i, uint8_t *zip, uint32_t *c)
{
    uLong l = (uLong) ((s->n + 2) * MIN(s->r, s->n + 2 - i) * s->c * s->b / 8);
    uLong z = compressBound(l);

    compress((Bytef *) zip, &z, (const Bytef *) bin, l);
    *c = (uint32_t) z;
}

static void fromzip(scm *s, uint8_t *bin, int i, uint8_t *zip, uint32_t c)
{
    uLong l = (uLong) ((s->n + 2) * MIN(s->r, s->n + 2 - i) * s->c * s->b / 8);
    uLong z = (uLong) c;

    uncompress((Bytef *) bin, &l, (const Bytef *) zip, z);
}

//------------------------------------------------------------------------------

// Read a page of data into the zip caches. Store the strip offsets and lengths
// in the given arrays. This is the serial part of the parallel input handler.

int scm_read_cache(scm *s, uint8_t **zv,
                           uint64_t oo,
                           uint64_t lo,
                           uint16_t sc, uint64_t *o, uint32_t *l)
{
    // Read the strip offset and length arrays.

    if (scm_read(s, o, sc * sizeof (uint64_t), oo) == -1) return -1;
    if (scm_read(s, l, sc * sizeof (uint32_t), lo) == -1) return -1;

    // Read each strip.

    for (int i = 0; i < sc; i++)
        if (scm_read(s, zv[i], (size_t) l[i], (off_t) o[i]) == -1)
            return -1;

    return 1;
}

// Write a page of data from the zip caches. This is the serial part of the
// parallel output handler. Also, in concert with scm_read_cache, this function
// allows data to be copied from one SCM to another without the computational
// cost of an unnecessary encode-decode cycle.

int scm_write_cache(scm *s, uint8_t **zv,
                            uint64_t *oo,
                            uint64_t *lo,
                            uint16_t *sc, uint64_t *o, uint32_t *l)
{
    int   c = (int) (*sc);
    off_t t;

    // Write each strip to the file, noting all offsets.

    for (int i = 0; i < c; i++)
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

    if (scm_read_cache(s, s->zipv, oo, lo, sc, o, l) > 0)
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

    return scm_write_cache(s, s->zipv, oo, lo, sc, o, l);
}

//------------------------------------------------------------------------------

// Set IFD c to be the "next" of IFD p. If p is zero, set IFD c to be the first
// IFD linked-to by the header.

int scm_link_list(scm *s, off_t c, off_t p)
{
    header h;
    ifd    i;

    if (p)
    {
        if (scm_read_ifd(s, &i, p) == 1)
        {
            i.next = c;

            if (scm_write_ifd(s, &i, p) == 1)
            {
                return 1;
            }
            else apperr("Failed to write previous IFD");
        }
        else apperr("Failed to read previous IFD");

        return -1;
    }
    else
    {
        if (scm_read_header(s, &h) == 1)
        {
            h.first_ifd = c;

            if (scm_write_header(s, &h) == 1)
            {
                return 1;
            }
            else apperr("Failed to write header");
        }
        else apperr("Failed to write header");

        return -1;
    }
}

// Set IFD c to be the nth "child" of IFD p.

int scm_link_tree(scm *s, off_t c, off_t p, int n)
{
    ifd i;

    if (p)
    {
        if (scm_read_ifd(s, &i, p) == 1)
        {
            i.sub[n] = c;

            if (scm_write_ifd(s, &i, p) == 1)
            {
                return 1;
            }
            else apperr("Failed to write parent IFD");
        }
        else apperr("Failed to read parent IFD");

        return -1;
    }
    return 0;
}

//------------------------------------------------------------------------------
