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

    int n;                    // Page sample count excluding border
    int d;                    // Page subdivision count
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
    struct field copyright;

    uint64_t next;

    uint64_t ifds[4];
    char     date[20];
};

#pragma pack(pop)

//------------------------------------------------------------------------------

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
// IFD. d gives the data buffer size. t gives the text buffer size. s gives the
// image width and height. c gives the image channel count. b gives the image
// byte count per channel. Some fields are assigned magic numbers defined in the
// TIFF Specification Revision 6.0.

static void set_ifd(struct ifd *ip, off_t o, size_t d, size_t t,
                                   size_t s, size_t c, size_t b)
{
    assert(s > 0);
    assert(c > 0);
    assert(c < 5);
    assert(b > 0);

    const size_t oi = o + offsetof (struct ifd, ifds);
    const size_t od = o + offsetof (struct ifd, date);
    const size_t os = o +   sizeof (struct ifd);

    ip->count   = 15;
    ip->next    =  0;
    ip->ifds[0] =  0;
    ip->ifds[1] =  0;
    ip->ifds[2] =  0;
    ip->ifds[3] =  0;

    set_field(&ip->subfile_type,      0x00FE,  4,  1,  2);
    set_field(&ip->image_width,       0x0100,  3,  1,  s);
    set_field(&ip->image_length,      0x0101,  3,  1,  s);
    set_field(&ip->bits_per_sample,   0x0102,  3,  c,  0);
    set_field(&ip->compression,       0x0103,  3,  1,  8);
    set_field(&ip->interpretation,    0x0106,  3,  1, c == 1 ? 1 : 2);
    set_field(&ip->strip_offsets,     0x0111, 16,  1, os);
    set_field(&ip->orientation,       0x0112,  3,  1,  2);
    set_field(&ip->samples_per_pixel, 0x0115,  3,  1,  c);
    set_field(&ip->strip_byte_counts, 0x0117,  4,  1,  d);
    set_field(&ip->configuration,     0x011C,  3,  1,  1);
    set_field(&ip->datetime,          0x0132,  2, 20, od);
    set_field(&ip->predictor,         0x013D,  3,  1,  2);
    set_field(&ip->sub_ifds,          0x014A, 18,  4, oi);
    set_field(&ip->copyright,         0x8298,  2,  t, sizeof (struct header));

    // Since c < 5, the bits-per-sample field fits within its own offset.

    uint16_t *bps = (uint16_t *) (&ip->bits_per_sample.offset);

    bps[0] = (uint16_t) ((0 < c) ? b : 0);
    bps[1] = (uint16_t) ((1 < c) ? b : 0);
    bps[2] = (uint16_t) ((2 < c) ? b : 0);
    bps[3] = (uint16_t) ((3 < c) ? b : 0);

    // Encode the current date and time as ASCII text.

    const time_t now = time(0);
    strftime(ip->date, 20, "%Y:%m:%d %H:%M:%S", localtime(&now));
}

//------------------------------------------------------------------------------

scm *scm_ofile(const char *name, int n, int d, int c, int b, int s)
{
    FILE *fp = NULL;
    scm  *sp = NULL;

    assert(name);
    assert(n > 0);
    assert(c > 0);
    assert(b > 0);

    if ((fp = fopen(name, "wb")))
    {
        if ((sp = (scm *) calloc(sizeof (scm), 1)))
        {
            sp->fp = fp;
            sp->n  = n;
            sp->d  = d;
            sp->c  = c;
            sp->b  = b;
            sp->s  = s;
        }
        else sys_err_mesg("Failed to allocate SCM '%s'", name);
    }
    else sys_err_mesg("Failed to open output '%s'", name);

    return sp;
}

scm *scm_ifile(const char *name)
{
    FILE *fp = NULL;
    scm  *sp = NULL;

    assert(name);

    if ((fp = fopen(name, "rb")))
    {
        if ((sp = (scm *) calloc(sizeof (scm), 1)))
        {
            sp->fp = fp;

        }
        else sys_err_mesg("Failed to allocate SCM '%s'", name);
    }
    else sys_err_mesg("Failed to open input '%s'", name);

    return sp;
}

//------------------------------------------------------------------------------

int scm_get_n(scm *sp)
{
    assert(sp);
    return sp->n;
}

int scm_get_d(scm *sp)
{
    assert(sp);
    return sp->d;
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

const char *scm_get_copyright(scm *sp)
{
    assert(sp);
    return sp->copyright;
}

void scm_set_copyright(scm *sp, const char *str)
{
    assert(sp);
    assert(str);

    char *cpy;

    if ((cpy = (char *) malloc(strlen(str) + 1)))
    {
        strcpy(cpy, str);
        free(sp->copyright);
        sp->copyright = cpy;
    }
}

//------------------------------------------------------------------------------
