// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <zlib.h>

#include "scm.h"
#include "error.h"

//------------------------------------------------------------------------------

struct scm
{
    FILE *fp;                 // I/O file pointer
    char *copyright;          // Copyright string

    int n;                    // Page sample count
    int c;                    // Sample channel count
    int b;                    // Channel bit count
    int s;                    // Channel signed flag

    void  *bin;               // Bin scratch buffer pointer
    void  *zip;               // Zip scratch buffer pointer
};

//------------------------------------------------------------------------------
// The following structures define the format of an SCM TIFF: a BigTIFF with a
// specific set of fifteen fields in each IFD. LibTIFF4 has no trouble reading
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

    field subfile_type;
    field image_width;
    field image_length;
    field bits_per_sample;
    field compression;
    field interpretation;
    field strip_offsets;
    field orientation;
    field samples_per_pixel;
    field strip_byte_counts;
    field configuration;
    field datetime;
    field predictor;
    field sub_ifds;
    field sample_format;
    field copyright;

    uint64_t next;

    uint64_t ifds[4];
    char     date[20];
};

#pragma pack(pop)

//------------------------------------------------------------------------------

static size_t align2(size_t t)
{
    return (t & 1) ? (t + 1) : t;
}

// Initialize an SCM TIFF header. TIFF copyright text is specified per IFD, but
// SCM TIFF copyright is the same for all IFDs and the text is included in the
// file exactly once, immediately following the header. The length of this text
// must be a multiple of two to meet TIFF alignment requirements.

static void set_header(header *hp, size_t t)
{
    assert((t & 1) == 0);

    hp->endianness = 0x4949;
    hp->version    = 0x002B;
    hp->offsetsize = 8;
    hp->zero       = 0;
    hp->first_ifd  = 0;
}

// Initialize an SCM TIFF field.

static void set_field(field *fp, int t, int y, size_t c, off_t o)
{
    fp->tag    = (uint16_t) t;
    fp->type   = (uint16_t) y;
    fp->count  = (uint64_t) c;
    fp->offset = (uint64_t) o;
}

// Initialize an SCM TIFF Image File Directory. o gives the file offset of this
// IFD. d gives the data buffer size. t gives the text buffer size. n gives the
// image width and height. c gives the image channel count. b gives the image
// bit count per channel. Some fields are assigned magic numbers defined in the
// TIFF Specification Revision 6.0.

static void set_ifd(ifd *ip, off_t o, size_t d, size_t t,
                             int n, int c, int b, int s)
{
    assert(n > 0);
    assert(c > 0);
    assert(c < 5);
    assert(b > 0);

    const size_t oi = o + offsetof (ifd, ifds);
    const size_t od = o + offsetof (ifd, date);
    const size_t os = o +   sizeof (ifd);

    ip->count   = 16;
    ip->next    =  0;
    ip->ifds[0] =  0;
    ip->ifds[1] =  0;
    ip->ifds[2] =  0;
    ip->ifds[3] =  0;

    set_field(&ip->subfile_type,      0x00FE,  4,  1,  2);
    set_field(&ip->image_width,       0x0100,  3,  1,  n + 2);
    set_field(&ip->image_length,      0x0101,  3,  1,  n + 2);
    set_field(&ip->bits_per_sample,   0x0102,  3,  c,  0);
    set_field(&ip->compression,       0x0103,  3,  1,  8);
    set_field(&ip->interpretation,    0x0106,  3,  1, (c == 1) ? 1 : 2);
    set_field(&ip->strip_offsets,     0x0111, 16,  1, os);
    set_field(&ip->orientation,       0x0112,  3,  1,  2);
    set_field(&ip->samples_per_pixel, 0x0115,  3,  1,  c);
    set_field(&ip->strip_byte_counts, 0x0117,  4,  1,  d);
    set_field(&ip->configuration,     0x011C,  3,  1,  1);
    set_field(&ip->datetime,          0x0132,  2, 20, od);
    set_field(&ip->predictor,         0x013D,  3,  1,  1); // 2
    set_field(&ip->sub_ifds,          0x014A, 18,  4, oi);
    set_field(&ip->sample_format,     0x0153,  3,  c,  0);
    set_field(&ip->copyright,         0x8298,  2,  t, sizeof (header));

    uint16_t *p;

    // Since c < 5, the bits-per-sample field fits within its own offset.

    p = (uint16_t *) (&ip->bits_per_sample.offset);

    p[0] = (uint16_t) b;
    p[1] = (uint16_t) b;
    p[2] = (uint16_t) b;
    p[3] = (uint16_t) b;

    // Since c < 5, the sample-format field fits within its own offset.

    p = (uint16_t *) (&ip->sample_format.offset);

    p[0] = (uint16_t) (s ? 2 : 1);
    p[1] = (uint16_t) (s ? 2 : 1);
    p[2] = (uint16_t) (s ? 2 : 1);
    p[3] = (uint16_t) (s ? 2 : 1);

    // Encode the current date and time as ASCII text.

    const time_t now = time(0);
    strftime(ip->date, 20, "%Y:%m:%d %H:%M:%S", localtime(&now));
}

//------------------------------------------------------------------------------

static int is_header(header *hp)
{
    return (hp->endianness == 0x4949
        &&  hp->version    == 0x002B
        &&  hp->offsetsize == 0x0008
        &&  hp->zero       == 0x0000);
}

static int is_ifd(ifd *ip)
{
    return (ip->count == 16

        &&  ip->subfile_type.tag      == 0x00FE
        &&  ip->image_width.tag       == 0x0100
        &&  ip->image_length.tag      == 0x0101
        &&  ip->bits_per_sample.tag   == 0x0102
        &&  ip->compression.tag       == 0x0103
        &&  ip->interpretation.tag    == 0x0106
        &&  ip->strip_offsets.tag     == 0x0111
        &&  ip->orientation.tag       == 0x0112
        &&  ip->samples_per_pixel.tag == 0x0115
        &&  ip->strip_byte_counts.tag == 0x0117
        &&  ip->configuration.tag     == 0x011C
        &&  ip->datetime.tag          == 0x0132
        &&  ip->predictor.tag         == 0x013D
        &&  ip->sub_ifds.tag          == 0x014A
        &&  ip->sample_format.tag     == 0x0153
        &&  ip->copyright.tag         == 0x8298);
}

//------------------------------------------------------------------------------

// Read and return a newly-allocated copy of the copyright string.

static char *get_ifd_copyright(ifd *ip, FILE *fp)
{
    const size_t len = ip->copyright.count;
    const off_t  off = ip->copyright.offset;

    char  *str;

    if ((str = (char *) calloc(len + 1, 1)))
    {
        if (fseeko(fp, off, SEEK_SET) == 0)
        {
            if (fread(str, 1, len, fp) == len)
            {
                return str;
            }
            else syserr("Failed to read copyright text");
        }
        else syserr("Failed to seek copyright text");
    }
    else syserr("Failed to allocate copyright text");

    free(str);
    return NULL;
}

// Validate and return the page size.

static int get_ifd_n(ifd *ip)
{
    if (ip->image_width.offset != ip->image_length.offset)
        apperr("TIFF page is not square");

    return (int) ip->image_width.offset - 2;
}

// Validate and return the channel count.

static int get_ifd_c(ifd *ip)
{
    if (ip->samples_per_pixel.offset < 1 ||
        ip->samples_per_pixel.offset > 4)
        apperr("Unsupported sample count");

    return (int) ip->samples_per_pixel.offset;
}

// Validate and return the channel depth.

static int get_ifd_b(ifd *ip)
{
    uint16_t *p = (uint16_t *) (&ip->bits_per_sample.offset);

    if ((ip->samples_per_pixel.offset > 1 && p[1] != p[0]) ||
        (ip->samples_per_pixel.offset > 2 && p[2] != p[0]) ||
        (ip->samples_per_pixel.offset > 3 && p[3] != p[0]))
        apperr("TIFF samples do not have same depth");

    return (int) p[0];
}

// Validate and return the signedness.

static int get_ifd_s(ifd *ip)
{
    uint16_t *p = (uint16_t *) (&ip->sample_format.offset);

    if ((ip->samples_per_pixel.offset > 1 && p[1] != p[0]) ||
        (ip->samples_per_pixel.offset > 2 && p[2] != p[0]) ||
        (ip->samples_per_pixel.offset > 3 && p[3] != p[0]))
        apperr("TIFF samples do not have same signedness");

    if      (p[0] == 1) return 0;
    else if (p[0] == 2) return 1;
    else apperr("Unknown sample format");

    return 0;
}

//------------------------------------------------------------------------------

// Encode the n values in floating point buffer f to the raw buffer p with
// b bits per sample and sign s.

static void ftob(void *p, const double *f, size_t n, int b, int s)
{
    size_t i;

    if      (b ==  8 && s == 0)
        for (i = 0; i < n; ++i)
            ((unsigned char  *) p)[i] = (unsigned char)  (f[i] * 255);

    else if (b == 16 && s == 0)
        for (i = 0; i < n; ++i)
            ((unsigned short *) p)[i] = (unsigned short) (f[i] * 65535);

    else if (b ==  8 && s == 1)
        for (i = 0; i < n; ++i)
            ((char  *) p)[i] = (char)  (f[i] * 127);

    else if (b == 16 && s == 1)
        for (i = 0; i < n; ++i)
            ((short *) p)[i] = (short) (f[i] * 32767);

}

// Decode the n values in raw buffer p to the floating point buffer f assuming
// b bits per sample and sign s.

static void btof(const void *p, double *f, size_t n, int b, int s)
{
    size_t i;

    if      (b ==  8 && s == 0)
        for (i = 0; i < n; ++i)
            f[i] = ((unsigned char  *) p)[i] / 255.0;

    else if (b == 16 && s == 0)
        for (i = 0; i < n; ++i)
            f[i] = ((unsigned short *) p)[i] / 65535.0;

    else if (b ==  8 && s == 1)
        for (i = 0; i < n; ++i)
            f[i] = ((char  *) p)[i] / 127.0;

    else if (b == 16 && s == 1)
        for (i = 0; i < n; ++i)
            f[i] = ((short *) p)[i] / 32767.0;
}

static void hdif(void *p, size_t n)
{
}

//------------------------------------------------------------------------------

// Allocate properly-sized bin and zip scratch buffers for SCM s.

static int scm_alloc(scm *s)
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

// Ensure that the current SCM TIFF position falls on a TIFF word boundary.

static off_t scm_align(scm *s)
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
    else syserr("Failed to seek SCM");

    return -1;
}

// Read the SCM TIFF header.

static int scm_read_header(scm *s, header *h)
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

static int scm_write_header(scm *s, header *h)
{
    assert(s);
    assert(h);

    if (fseeko(s->fp, 0, SEEK_SET) == 0)
    {
        if (fwrite(h, sizeof (header), 1, s->fp) == 1)
        {
            return 1;
        }
        else syserr("Failed to read SCM header");
    }
    else syserr("Failed to seek SCM");

    return -1;
}

// Read an IFD at offset o of SCM TIFF s.

static int scm_read_ifd(scm *s, ifd *i, off_t o)
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

static int scm_write_ifd(scm *s, ifd *i, off_t o)
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

// Set IFD c to be the "next" of IFD p. If p is zero, set IFD c to be the first
// IFD linked-to by the header.

static int scm_link_list(scm *s, off_t c, off_t p)
{
    header h;
    ifd    i;

    if (p)
    {
        if (scm_read_ifd(s, &i, p))
        {
            i.next = c;

            if (scm_write_ifd(s, &i, p))
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
        if (scm_read_header(s, &h))
        {
            h.first_ifd = c;

            if (scm_write_header(s, &h))
            {
                return 1;
            }
            else apperr("Failed to write header");
        }
        else apperr("Failed to write header");
    }
    return 0;
}

// Set IFD c to be the nth "child" of IFD p.

static int scm_link_tree(scm *s, off_t c, off_t p, int n)
{
    ifd i;

    if (p)
    {
        if (scm_read_ifd(s, &i, p))
        {
            i.ifds[n] = c;

            if (scm_write_ifd(s, &i, p))
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

// Encode, compress, and write buffer p at the current position in SCM TIFF s.

static size_t scm_write_data(scm *s, const double *p)
{
    const size_t l = (s->n + 2) * (s->n + 2) * s->c * s->b / 8;
    const size_t n = (s->n + 2) * (s->n + 2) * s->c;

    ftob(s->bin, p, n, s->b, s->s);
    hdif(s->bin, n);

    uLong z = compressBound(l);

    if (compress(s->zip, &z, s->bin, l) == Z_OK)
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

// Read, uncompress, and decode the data at the current position in SCM TIFF s.

static size_t scm_read_data(scm *s, double *p, size_t z)
{
    const size_t l = (s->n + 2) * (s->n + 2) * s->c * s->b / 8;
    const size_t n = (s->n + 2) * (s->n + 2) * s->c;

    uLong b = (uLong) l;

    if (fread(s->zip, 1, z, s->fp) == z)
    {
        if (uncompress(s->bin, &b, s->zip, z) == Z_OK)
        {
            hdif(s->bin, n);
            btof(s->bin, p, n, s->b, s->s);

            return b;
        }
        else apperr("Failed to uncompress data");
    }
    else syserr("Failed to read compressed data");

    return 0;
}

//------------------------------------------------------------------------------

void scm_close(scm *s)
{
    if (s)
    {
        fclose(s->fp);
        free(s->copyright);
        free(s->bin);
        free(s->zip);
        free(s);
    }
}

// Open an SCM TIFF output file. Initialize and return an SCM structure with the
// given parameters. Write the TIFF header with padded copyright text.

scm *scm_ofile(const char *name, int n, int c, int b, int g, const char *text)
{
    FILE *fp = NULL;
    scm  *s  = NULL;

    assert(name);
    assert(n > 0);
    assert(c > 0);
    assert(c < 5);
    assert(b > 0);
    assert(text);

    const  size_t t = align2(strlen(text));
    header h;

    set_header(&h, t);

    if ((fp = fopen(name, "w+b")))
    {
        if (fwrite(&h, sizeof (h), 1, fp) == 1)
        {
            if (fwrite(text, 1, t, fp) == t)
            {
                if ((s = (scm *) calloc(sizeof (scm), 1)))
                {
                    s->fp = fp;
                    s->n  = n;
                    s->c  = c;
                    s->b  = b;
                    s->s  = g;

                    if (scm_alloc(s))
                    {
                        if ((s->copyright = calloc(t + 1, 1)))
                        {
                            strcpy(s->copyright, text);
                            return s;
                        }
                        else syserr("Failed to allocate copyright text");
                    }
                    else syserr("Failed to allocate SCM scratch buffers");
                }
                else syserr("Failed to allocate SCM");
            }
            else syserr("Failed to write '%s'", name);
        }
        else syserr("Failed to write '%s'", name);
    }
    else syserr("Failed to open '%s'", name);

    scm_close(s);
    return NULL;
}

// Open an SCM TIFF input file. Validate the header. Read and validate the first
// IFD. Initialize and return an SCM structure using the first IFD's parameters.

scm *scm_ifile(const char *name)
{
    FILE *fp = NULL;
    scm  *s  = NULL;

    header h;
    ifd    i;

    assert(name);

    if ((fp = fopen(name, "rb")))
    {
        if (fread(&h, sizeof (h), 1, fp) == 1)
        {
            if (is_header(&h))
            {
                if ((s = (scm *) calloc(sizeof (scm), 1)))
                {
                    s->fp = fp;

                    if (scm_read_ifd(s, &i, (off_t) h.first_ifd) == 1)
                    {
                        s->copyright = get_ifd_copyright(&i, fp);
                        s->n         = get_ifd_n(&i);
                        s->c         = get_ifd_c(&i);
                        s->b         = get_ifd_b(&i);
                        s->s         = get_ifd_s(&i);

                        if (scm_alloc(s))
                        {
                            return s;
                        }
                        else syserr("Failed to allocate SCM scratch buffers");
                    }
                    else syserr("Failed to read '%s' first IFD", name);
                }
                else syserr("Failed to allocate SCM");
            }
            else apperr("'%s' is not an SCM TIFF", name);
        }
        else syserr("Failed to read '%s'", name);
    }
    else syserr("Failed to open '%s'", name);

    scm_close(s);
    return NULL;
}

//------------------------------------------------------------------------------

// Append a page at the current SCM TIFF file pointer. IFD l gives the previous
// page in the breadth-first list. IFD p gives the parent page. The parent will
// be updated to include the new page as child n. dat points to a page of data
// to be written. Return the offset of the end of the new page (the next IFD).

off_t scm_append(scm *s, off_t l, off_t p, int n, const double *dat)
{
    assert(s);
    assert(dat);
    assert(n >= 0);
    assert(n <= 3);

    size_t t = s->copyright ? strlen(s->copyright) : 0;
    size_t d;
    off_t  o;
    ifd    i;

    if ((o = ftello(s->fp)) >= 0)
    {
        set_ifd(&i, o, 0, t, s->n, s->c, s->b, s->s);

        if (scm_write_ifd(s, &i, o) == 1)
        {
            if ((d = scm_write_data(s, dat)) > 0)
            {
                if (scm_align(s) >= 0)
                {
                    set_ifd(&i, o, d, t, s->n, s->c, s->b, s->s);

                    if (scm_write_ifd(s, &i, o) == 1)
                    {
                        if (scm_link_list(s, o, l) >= 0)
                        {
                            if (scm_link_tree(s, o, p, n) >= 0)
                            {
                                if (fseeko(s->fp, 0, SEEK_END) == 0)
                                {
                                    return o;
                                }
                                else syserr("Failed to seek SCM");
                            }
                            else apperr("Failed to link IFD tree");
                        }
                        else apperr("Failed to link IFD list");
                    }
                    else apperr("Failed to re-write IFD");
                }
                else syserr("Failed to align SCM");
            }
            else apperr("Failed to write data");
        }
        else apperr("Failed to pre-write IFD");
    }
    else syserr("Failed to tell SCM");

    return 0;
}

// Move the SCM TIFF file pointer to the first IFD and return its offset.

off_t scm_rewind(scm *s)
{
    header h;

    assert(s);

    if (scm_read_header(s, &h) == 1)
    {
        if (fseeko(s->fp, (off_t) h.first_ifd, SEEK_SET) == 0)
        {
            return (off_t) h.first_ifd;
        }
        else syserr("Failed to seek SCM TIFF");
    }
    else syserr("Failed to read SCM header");

    return 0;
}

//------------------------------------------------------------------------------

// Read the SCM TIFF IFD at offset o. Return the offset of the next IFD. If p is
// non-null, assume it can store four offsets and copy the SubIFDs there.

off_t scm_read_node(scm *s, off_t o, off_t *p)
{
    ifd i;

    assert(s);

    if (scm_read_ifd(s, &i, o) == 1)
    {
        if (p)
        {
            p[0] = (off_t) i.ifds[0];
            p[1] = (off_t) i.ifds[1];
            p[2] = (off_t) i.ifds[2];
            p[3] = (off_t) i.ifds[3];
        }
        return (off_t) i.next;
    }
    else apperr("Failed to read SCM TIFF IFD");

    return 0;
}

// Read the SCM TIFF IFD at offset o. Assume p provides space for one page of
// data to be stored.

size_t scm_read_page(scm *s, off_t o, double *p)
{
    ifd i;

    assert(s);

    if (scm_read_ifd(s, &i, o) == 1)
    {
        const int in = get_ifd_n(&i);
        const int ic = get_ifd_c(&i);
        const int ib = get_ifd_b(&i);
        const int is = get_ifd_s(&i);

        if (in == s->n && ic == s->c && ib == s->b && is == s->s)
        {
            return scm_read_data(s, p, (size_t) i.strip_byte_counts.offset);
        }
        else apperr("SCM TIFF IFD %llx data format (%i/%i/%i/%i) "
                                   "does not match (%i/%i/%i/%i)\n",
                               o, in, ic, ib, is, s->n, s->c, s->b, s->s);
    }
    else apperr("Failed to read SCM TIFF IFD");

    return 0;
}

//------------------------------------------------------------------------------

const char *scm_get_copyright(scm *sp)
{
    assert(sp);
    return sp->copyright;
}

int scm_get_n(scm *sp)
{
    assert(sp);
    return sp->n;
}

int scm_get_c(scm *sp)
{
    assert(sp);
    return sp->c;
}

int scm_get_b(scm *sp)
{
    assert(sp);
    return sp->b;
}

int scm_get_s(scm *sp)
{
    assert(sp);
    return sp->s;
}

//------------------------------------------------------------------------------
