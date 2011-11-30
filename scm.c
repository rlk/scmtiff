// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

#include "scm.h"
#include "error.h"

//------------------------------------------------------------------------------

struct scm
{
    FILE *fp;
    char *copyright;

    int n;                    // Page sample count
    int c;                    // Sample channel count
    int b;                    // Channel bit count
    int s;                    // Channel signed flag
};

//------------------------------------------------------------------------------
// The following structures define the format of an SCM TIFF: a BigTIFF with a
// specific set of fifteen fields in each IFD. LibTIFF4 has no trouble reading
// this format, but its SubIFD API is not sufficient to write it.

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

    struct field subfile_type;
    struct field image_width;
    struct field image_length;
    struct field bits_per_sample;
    struct field compression;
    struct field interpretation;
    struct field strip_offsets;
    struct field orientation;
    struct field samples_per_pixel;
    struct field strip_byte_counts;
    struct field configuration;
    struct field datetime;
    struct field predictor;
    struct field sub_ifds;
    struct field sample_format;
    struct field copyright;

    uint64_t next;

    uint64_t ifds[4];
    char     date[20];
};

#pragma pack(pop)

//------------------------------------------------------------------------------

static int align2(int t)
{
    return (t & 1) ? (t + 1) : t;
}

// Initialize an SCM TIFF header. TIFF copyright text is specified per IFD, but
// SCM TIFF copyright is the same for all IFDs and the text is included in the
// file exactly once, immediately following the header. The length of this text
// must be a multiple of two to meet TIFF alignment requirements.

static void set_header(struct header *hp, size_t t)
{
    assert((t & 1) == 0);

    hp->endianness = 0x4949;
    hp->version    = 0x002B;
    hp->offsetsize = 0x0008;
    hp->zero       = 0x0000;
    hp->first_ifd  = (uint64_t) (sizeof (struct header) + t);
}

// Initialize an SCM TIFF field.

static void set_field(struct field *fp, int t, int y, size_t c, off_t o)
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

static void set_ifd(struct ifd *ip, off_t o, size_t d, size_t t,
                                     int n, int c, int b, int s)
{
    assert(s > 0);
    assert(c > 0);
    assert(c < 5);
    assert(b > 0);

    const size_t oi = o + offsetof (struct ifd, ifds);
    const size_t od = o + offsetof (struct ifd, date);
    const size_t os = o +   sizeof (struct ifd);

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
    set_field(&ip->predictor,         0x013D,  3,  1,  2);
    set_field(&ip->sub_ifds,          0x014A, 18,  4, oi);
    set_field(&ip->sample_format,     0x0153,  3,  c,  0);
    set_field(&ip->copyright,         0x8298,  2,  t, sizeof (struct header));

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

static int is_header(struct header *hp)
{
    return (hp->endianness == 0x4949
        &&  hp->version    == 0x002B
        &&  hp->offsetsize == 0x0008
        &&  hp->zero       == 0x0000);
}

static int is_ifd(struct ifd *ip)
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

static char *get_ifd_copyright(struct ifd *ip, FILE *fp)
{
    const size_t len = ip->copyright.count;
    const off_t  off = ip->copyright.offset;

    char  *str;

    if ((str = (char *) calloc(len + 1, 1)))
    {
        if (fseeko(fp, SEEK_SET, off) == 0)
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

static int get_ifd_n(struct ifd *ip)
{
    if (ip->image_width.offset != ip->image_length.offset)
        apperr("TIFF page is not square");

    return (int) ip->image_width.offset;
}

// Validate and return the channel count.

static int get_ifd_c(struct ifd *ip)
{
    if (ip->samples_per_pixel.offset < 1 ||
        ip->samples_per_pixel.offset > 4)
        apperr("Unsupported sample count");

    return (int) ip->samples_per_pixel.offset;
}

// Validate and return the channel depth.

static int get_ifd_b(struct ifd *ip)
{
    uint16_t *p = (uint16_t *) (&ip->bits_per_sample.offset);

    if ((ip->samples_per_pixel.offset > 1 && p[1] != p[0]) ||
        (ip->samples_per_pixel.offset > 2 && p[2] != p[0]) ||
        (ip->samples_per_pixel.offset > 3 && p[3] != p[0]))
        apperr("TIFF samples do not have equal depth");

    return (int) p[0];
}

// Validate and return the signedness.

static int get_ifd_s(struct ifd *ip)
{
    uint16_t *p = (uint16_t *) (&ip->sample_format.offset);

    if ((ip->samples_per_pixel.offset > 1 && p[1] != p[0]) ||
        (ip->samples_per_pixel.offset > 2 && p[2] != p[0]) ||
        (ip->samples_per_pixel.offset > 3 && p[3] != p[0]))
        apperr("TIFF samples do not have equal signedness");

    if      (p[0] == 1) return 0;
    else if (p[0] == 2) return 1;
    else apperr("Unknown sample format");

    return 0;
}

//------------------------------------------------------------------------------

// Open an SCM TIFF output file. Initialize and return an SCM structure with the
// given parameters. Write the TIFF header with padded copyright text.

scm *scm_ofile(const char *name, int n, int c, int b, int s, const char *text)
{
    FILE *fp = NULL;
    scm  *sp = NULL;

    assert(name);
    assert(n > 0);
    assert(c > 0);
    assert(c < 5);
    assert(b > 0);
    assert(text);

    const  size_t t = align2(strlen(text));
    struct header h;

    set_header(&h, t);

    if ((fp = fopen(name, "wb")))
    {
        if (fwrite(&h, sizeof (h), 1, fp) == 1)
        {
            if (fwrite(text, 1, t, fp) == t)
            {
                if ((sp = (scm *) calloc(sizeof (scm), 1)))
                {
                    sp->fp = fp;
                    sp->n  = n;
                    sp->c  = c;
                    sp->b  = b;
                    sp->s  = s;
                }
                else syserr("Failed to allocate SCM");
            }
            else syserr("Failed to write '%s'", name);
        }
        else syserr("Failed to write '%s'", name);
    }
    else syserr("Failed to open '%s'", name);

    return sp;
}

// Open an SCM TIFF input file. Validate the header. Read and validate the first
// IFD. Initialize and return an SCM structure using the first IFD's parameters.

scm *scm_ifile(const char *name)
{
    FILE *fp = NULL;
    scm  *sp = NULL;

    struct header h;
    struct ifd    i;

    assert(name);

    if ((fp = fopen(name, "rb")))
    {
        if (fread(&h, sizeof (h), 1, fp) == 1)
        {
            if (is_header(&h))
            {
                if (fseeko(fp, (off_t) h.first_ifd, SEEK_SET) == 0)
                {
                    if (fread(&i, sizeof (i), 1, fp) == 1)
                    {
                        if (is_ifd(&i))
                        {
                            if ((sp = (scm *) calloc(sizeof (scm), 1)))
                            {
                                sp->copyright = get_ifd_copyright(&i, fp);
                                sp->n         = get_ifd_n(&i);
                                sp->c         = get_ifd_c(&i);
                                sp->b         = get_ifd_b(&i);
                                sp->s         = get_ifd_s(&i);
                                sp->fp        = fp;
                            }
                            else syserr("Failed to allocate SCM");
                        }
                        else apperr("'%s' is not an SCM TIFF", name);
                    }
                    else syserr("Failed to read '%s' IFD", name);
                }
                else syserr("Failed to seek '%s' IFD", name);
            }
            else apperr("'%s' is not an SCM TIFF", name);
        }
        else syserr("Failed to read '%s'", name);
    }
    else syserr("Failed to open '%s'", name);

    return sp;
}

void scm_close(scm *s)
{
    assert(s);
    fclose(s->fp);
    free(s->copyright);
    free(s);
}

//------------------------------------------------------------------------------

// Append a page at the current SCM TIFF file pointer. Offset o gives the parent
// page. The parent will be updated to include the new page as child x. Assume p
// points to a page of data to be converted to raw, compressed, and written.

off_t scm_append(scm *s, off_t o, int x, const double *p)
{
    return 0;
}

// Move the SCM TIFF file pointer to the first IFD and return its offset.

off_t scm_rewind(scm *s)
{
    struct header h;

    assert(s);

    if (fseeko(s->fp, 0, SEEK_SET) == 0)
    {
        if (fread(&h, sizeof (h), 1, s->fp) == 1)
        {
            if (is_header(&h))
            {
                if (fseeko(s->fp, (off_t) h.first_ifd, SEEK_SET) == 0)
                {
                    return (off_t) h.first_ifd;
                }
                else syserr("Failed to seeks SCM");
            }
            else apperr("File is not an SCM TIFF");
        }
        else syserr("Failed to read SCM");
    }
    else syserr("Failed to seek SCM");

    return 0;
}

//------------------------------------------------------------------------------

// Read the SCM TIFF IFD at offset o. Return the offset of the next IFD. If p is
// non-null, assume it provides storage for four offsets and copy the offsets of
// any child IFDs there.

off_t scm_read_head(scm *s, off_t o, off_t *p)
{
    struct ifd i;

    assert(s);

    if (fseeko(s->fp, o, SEEK_SET) == 0)
    {
        if (fread(&i, sizeof (i), 1, s->fp) == 1)
        {
            if (is_ifd(&i))
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
            else apperr("File is not an SCM TIFF");
        }
        else syserr("Failed to read SCM");
    }
    else syserr("Failed to seek SCM");

    return 0;
}

// Read the SCM TIFF IFD at offset o. If p is non-null, assume it provides space
// for one page of data to be decompressed, converted to double, and stored.

int scm_read_data(scm *s, off_t o, double *p)
{
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
