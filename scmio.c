// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zlib.h>

#include "scmdat.h"
#include "err.h"

//------------------------------------------------------------------------------

// Allocate properly-sized bin and zip scratch buffers for SCM s.

int scm_alloc(scm *s)
{
    size_t bs = (s->n + 2) * (s->n + 2) * s->c * s->b / 8;
    size_t zs = compressBound(bs);

    assert(bs);
    assert(zs);

    if ((s->bin = malloc(bs)))
    {
        if ((s->zip = malloc(zs)))
        {
            return 1;
        }
    }

    free(s->zip);
    s->zip = 0;
    free(s->bin);
    s->bin = 0;

    return 0;
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

            s->n = s->D.image_width.offset - 2;
            s->c = s->D.samples_per_pixel.offset;

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
        set_field(&s->D.configuration,     0x011C, 3, 1,    1);
        set_field(&s->D.predictor,         0x013D, 3, 1,    p);
        set_field(&s->D.sample_format,     0x0153, 3, s->c, 0);

        scm_write_field(s, &s->D.description,    str);
        scm_align(s);
        scm_write_field(s, &s->D.bits_per_sample, bv);
        scm_align(s);
        scm_write_field(s, &s->D.sample_format,   fv);
        scm_align(s);

        s->D.count = 16;

        return 1;
    }
    return -1;
}

//------------------------------------------------------------------------------

// Read, uncompress, and decode the data at the current position in SCM TIFF s.

size_t scm_read_data(scm *s, float *p, size_t z)
{
    const size_t l = (s->n + 2) * (s->n + 2) * s->c * s->b / 8;
    const size_t n = (s->n + 2) * (s->n + 2) * s->c;

    uLong b = (uLong) l;

    if (fread(s->zip, 1, z, s->fp) == z)
    {
        if (uncompress((Bytef *) s->bin, &b, (const Bytef *) s->zip, z) == Z_OK)
        {
            if (s->D.predictor.offset == 2)
                dehdif(s->bin, s->n + 2, s->c, s->b);

            btof(s->bin, p, n, s->b, s->g);

            return b;
        }
        else apperr("Failed to uncompress data");
    }
    else syserr("Failed to read compressed data");

    return 0;
}

// Encode, compress, and write buffer p at the current position in SCM TIFF s.

size_t scm_write_data(scm *s, const float *p)
{
    const size_t l = (s->n + 2) * (s->n + 2) * s->c * s->b / 8;
    const size_t n = (s->n + 2) * (s->n + 2) * s->c;

    ftob(s->bin, p, n, s->b, s->g);

    if (s->D.predictor.offset == 2)
        enhdif(s->bin, s->n + 2, s->c, s->b);

    uLong z = compressBound(l);

    if (compress((Bytef *) s->zip, &z, (const Bytef *) s->bin, l) == Z_OK)
    {
        if (fwrite(s->zip, 1, z, s->fp) == z)
        {
            return z;
        }
        else syserr("Failed to write compressed data");
    }
    else apperr("Failed to compress data");

    return 0;
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
