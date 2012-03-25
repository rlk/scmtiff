// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "scmdat.h"
#include "scmio.h"
#include "scm.h"
#include "err.h"
#include "util.h"

//------------------------------------------------------------------------------

// Release all resources associated with SCM s. This function may be used to
// clean up after an error during initialization, and does not assume that the
// structure is fully populated.

void scm_close(scm *s)
{
    if (s)
    {
        fclose(s->fp);
        scm_free(s);
        free(s->str);
        free(s);
    }
}

// Open an SCM TIFF input file. Validate the header. Read and validate the first
// IFD. Initialize and return an SCM structure using the first IFD's parameters.

scm *scm_ifile(const char *name)
{
    scm  *s  = NULL;

    assert(name);

    if ((s = (scm *) calloc(sizeof (scm), 1)))
    {
        if ((s->fp = fopen(name, "r+b")))
        {
            if (scm_read_preface(s) == 1)
            {
                if (scm_alloc(s))
                {
                    return s;
                }
                else syserr("Failed to allocate SCM scratch buffers");
            }
            else syserr("Failed to read '%s'", name);
        }
        else syserr("Failed to open '%s'", name);
    }
    else syserr("Failed to allocate SCM");

    scm_close(s);
    return NULL;
}

// Open an SCM TIFF output file. Initialize and return an SCM structure with the
// given parameters. Write the TIFF header and SCM TIFF preface.

scm *scm_ofile(const char *name, int n, int c, int b, int g, const char *str)
{
    scm *s  = NULL;

    assert(name);
    assert(n > 0);
    assert(c > 0);
    assert(b > 0);
    assert(str);

    if ((s = (scm *) calloc(sizeof (scm), 1)))
    {
        s->str = strdup(str);
        s->n   = n;
        s->c   = c;
        s->b   = b;
        s->g   = g;
        s->r   = 16;

        if ((s->fp = fopen(name, "w+b")))
        {
            if (scm_write_preface(s, str))
            {
                if (scm_alloc(s))
                {
                    return s;
                }
                else syserr("Failed to allocate SCM scratch buffers");
            }
            else syserr("Failed to write '%s' preface", name);
        }
        else syserr("Failed to open '%s'", name);
    }
    else syserr("Failed to allocate SCM");

    scm_close(s);
    return NULL;
}

//------------------------------------------------------------------------------

// Append a page at the current SCM TIFF file pointer. Offset b is the previous
// IFD, which will be updated to include the new page as next. x is the breadth-
// first page index. f points to a page of data to be written. Return the offset
// of the new page.

long long scm_append(scm *s, long long b, long long x, const float *f)
{
    assert(s);
    assert(f);

    ifd D = s->D;
    long long o;

    uint64_t oo;
    uint64_t lo;
    uint16_t sc;

    if ((o = ftello(s->fp)) >= 0)
    {
        if (scm_write_ifd(s, &D, o) == 1)
        {
            if ((scm_write_data(s, f, &oo, &lo, &sc)) > 0)
            {
                if (scm_align(s) >= 0)
                {
                    uint64_t os = (uint64_t) o + (uint64_t) offsetof(ifd, sub);
                    uint64_t rr = (uint64_t) s->r;
                    uint64_t xx = (uint64_t) x;

                    set_field(&D.strip_offsets,     0x0111, 16, sc, oo);
                    set_field(&D.rows_per_strip,    0x0116,  3,  1, rr);
                    set_field(&D.strip_byte_counts, 0x0117,  4, sc, lo);
                    set_field(&D.sub_ifds,          0x014A, 18,  4, os);
                    set_field(&D.page_index,        0xFFB1,  4,  1, xx);

                    if (scm_write_ifd(s, &D, o) == 1)
                    {
                        if (scm_link_list(s, o, b) >= 0)
                        {
                            if (fseeko(s->fp, 0, SEEK_END) == 0)
                            {
                                fflush(s->fp);
                                return o;
                            }
                            else syserr("Failed to seek SCM");
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

// Repeat a page at the current file pointer of SCM s. As with append, offset b
// is the previous IFD, which will be updated to include the new page as next.
// The source data is at offset o of SCM t. SCMs s and t must have the same data
// type and size, as this allows the operation to be performed without decoding
// s or encoding t. If data types do not match, then a read from s and an append
// to t are required.

long long scm_repeat(scm *s, long long b, scm *t, long long o)
{
    assert(s);
    assert(t);
    assert(s->n == t->n);
    assert(s->c == t->c);
    assert(s->b == t->b);
    assert(s->g == t->g);
    assert(s->r == t->r);

    ifd D;

    if (scm_read_ifd(t, &D, o) == 1)
    {
        uint64_t oo = (uint64_t) D.strip_offsets.offset;
        uint64_t lo = (uint64_t) D.strip_byte_counts.offset;
        uint16_t sc = (uint16_t) D.strip_byte_counts.count;

        uint64_t O[sc];
        uint32_t L[sc];

        if (scm_read_cache(t, t->zipv, oo, lo, sc, O, L) > 0)
        {
            if ((o = ftello(s->fp)) >= 0)
            {
                if (scm_write_ifd(s, &D, o) == 1)
                {
                    if (scm_write_cache(s, t->zipv, &oo, &lo, &sc, O, L) > 0)
                    {
                        if (scm_align(s) >= 0)
                        {
                            uint64_t os = (uint64_t) o + (uint64_t) offsetof(ifd, sub);
                            uint64_t rr = (uint64_t) s->r;

                            set_field(&D.strip_offsets,     0x0111, 16, sc, oo);
                            set_field(&D.rows_per_strip,    0x0116,  3,  1, rr);
                            set_field(&D.strip_byte_counts, 0x0117,  4, sc, lo);
                            set_field(&D.sub_ifds,          0x014A, 18,  4, os);

                            if (scm_write_ifd(s, &D, o) == 1)
                            {
                                if (scm_link_list(s, o, b) >= 0)
                                {
                                    if (fseeko(s->fp, 0, SEEK_END) == 0)
                                    {
                                        fflush(s->fp);
                                        return o;
                                    }
                                    else syserr("Failed to seek SCM");
                                }
                                else apperr("Failed to link IFD list");
                            }
                            else apperr("Failed to re-write IFD");
                        }
                        else syserr("Failed to align SCM");
                    }
                    else apperr("Failed to write compressed data");
                }
                else apperr("Failed to pre-write IFD");
            }
            else syserr("Failed to tell SCM");
        }
        else syserr("Failed to read compressed data");
    }
    else apperr("Failed to read IFD");

    return 0;
}

// Move the SCM TIFF file pointer to the first IFD and return its offset.

long long scm_rewind(scm *s)
{
    header h;

    assert(s);

    if (scm_read_header(s, &h) == 1)
    {
        if (fseeko(s->fp, (long long) h.first_ifd, SEEK_SET) == 0)
        {
            return (long long) h.first_ifd;
        }
        else syserr("Failed to seek SCM TIFF");
    }
    else syserr("Failed to read SCM header");

    return 0;
}

// Scan the offset and page index of each IFD and rewrite all SubIFD fields.
// This has the effect of making the data structure depth-first seek-able
// without the need for a cataloging and mapping pre-pass. We may safely
// assume that this process does not change the length of the file or any
// offsets within it.

void scm_relink(scm *s)
{
    long long *a;
    int d;
    ifd i;

    assert(s);

    if ((d = scm_mapping(s, &a)))
    {
    	long long e = scm_get_page_count(d);

        for (long long x = 0; x < e; ++x)
            if (a[x] && scm_read_ifd(s, &i, a[x]) == 1)
            {
                long long x0 = scm_get_page_child(x, 0);
                long long x1 = scm_get_page_child(x, 1);
                long long x2 = scm_get_page_child(x, 2);
                long long x3 = scm_get_page_child(x, 3);

                i.sub[0] = (x0 < e) ? (uint64_t) a[x0] : 0;
                i.sub[1] = (x1 < e) ? (uint64_t) a[x1] : 0;
                i.sub[2] = (x1 < e) ? (uint64_t) a[x2] : 0;
                i.sub[3] = (x1 < e) ? (uint64_t) a[x3] : 0;

                scm_write_ifd(s, &i, a[x]);
            }

        free(a);
    }
}

//------------------------------------------------------------------------------

// Read the SCM TIFF IFD at offset o. Return this IFD's page index. If n is not
// null, store the next IFD offset there. If v is not null, assume it can store
// four offsets and copy the SubIFDs there.

long long scm_read_node(scm *s, long long o, long long *n, long long *v)
{
    ifd i;

    assert(s);

    if (o)
    {
        if (scm_read_ifd(s, &i, o) == 1)
        {
            if (n)
                n[0] = (long long) i.next;
            if (v)
            {
                v[0] = (long long) i.sub[0];
                v[1] = (long long) i.sub[1];
                v[2] = (long long) i.sub[2];
                v[3] = (long long) i.sub[3];
            }
            return (long long) i.page_index.offset;
        }
        else apperr("Failed to read SCM TIFF IFD");
    }
    return -1;
}

// Read the SCM TIFF IFD at offset o. Assume p provides space for one page of
// data to be stored.

bool scm_read_page(scm *s, long long o, float *p)
{
    ifd i;

    assert(s);

    if (scm_read_ifd(s, &i, o) == 1)
    {
        uint64_t oo = (uint64_t) i.strip_offsets.offset;
        uint64_t lo = (uint64_t) i.strip_byte_counts.offset;
        uint16_t sc = (uint16_t) i.strip_byte_counts.count;

        return (scm_read_data(s, p, oo, lo, sc) > 0);
    }
    else apperr("Failed to read SCM TIFF IFD");

    return false;
}

// Determine the file offset and page index of each IFD in SCM s. Return these
// in a newly-allocated array of offsets, indexed by page index. The allocated
// length of this array is sufficient to store a full tree, regardless of the
// true sparseness of SCM s.

int scm_mapping(scm *s, long long **a)
{
    long long l = 0;
    long long x = 0;
    long long o;
    long long n;

    assert(s);

    // Determine the largest page index.

    for (o = scm_rewind(s); (x = scm_read_node(s, o, &n, 0)) >= 0; o = n)
        if (l < x)
            l = x;

    // Determine the index and offset of each page and initialize a mapping.

    int    d =          scm_get_page_depth(l);
    size_t m = (size_t) scm_get_page_count(d);

    if ((*a = (long long *) calloc(m, sizeof (long long))))
    {
        for (o = scm_rewind(s); (x = scm_read_node(s, o, &n, 0)) >= 0; o = n)
            (*a)[x] = o;

        return d;
    }
    return -1;
}

//------------------------------------------------------------------------------

long long scm_read_catalog(scm *s, long long **a)
{
    long long o;
    long long n;
    long long c;

    ifd i;

    // Scan the file to determine the number of pages.

    for (c = 0, o = scm_rewind(s); scm_read_ifd(s, &i, o); o = n)
    {
        n = (long long) i.next;
        c++;
    }

    // Allocate and populate an array giving page indices and offsets.

    if ((a[0] = (long long *) malloc((size_t) (2 * c) * sizeof (long long))))
    {
        for (c = 0, o = scm_rewind(s); scm_read_ifd(s, &i, o); o = n)
        {
            n = (long long) i.next;
            a[0][2 * c + 0] = n;
            a[0][2 * c + 1] = o;
            c++;
        }
        return c;
    }
    return 0;
}

//------------------------------------------------------------------------------

// long long scm_read_catalog(scm *s, long long **p)
// {
//     return 0;
// }

//------------------------------------------------------------------------------

// Allocate and return a buffer with the proper size to fit one page of data,
// as determined by the parameters of SCM s, assuming float precision samples.

float *scm_alloc_buffer(scm *s)
{
    size_t o = (size_t) s->n + 2;
    size_t c = (size_t) s->c;

    return (float *) malloc(o * o * c * sizeof (float));
}

// Query the parameters of SCM s.

char *scm_get_description(scm *s)
{
    assert(s);
    return s->str;
}

int scm_get_n(scm *s)
{
    assert(s);
    return s->n;
}

int scm_get_c(scm *s)
{
    assert(s);
    return s->c;
}

int scm_get_b(scm *s)
{
    assert(s);
    return s->b;
}

int scm_get_g(scm *s)
{
    assert(s);
    return s->g;
}

//------------------------------------------------------------------------------

static long long log2i(long long n)
{
    long long r = 0;

    if (n >= (1LL << 32)) { n >>= 32; r += 32; }
    if (n >= (1LL << 16)) { n >>= 16; r += 16; }
    if (n >= (1LL <<  8)) { n >>=  8; r +=  8; }
    if (n >= (1LL <<  4)) { n >>=  4; r +=  4; }
    if (n >= (1LL <<  2)) { n >>=  2; r +=  2; }
    if (n >= (1LL <<  1)) {           r +=  1; }

    return r;
}

// Calculate the number of pages in an SCM of depth d.

long long scm_get_page_count(long long d)
{
    return (1 << (2 * d + 3)) - 2;
}

// Traverse up the tree to find the root page of page i.

long long scm_get_page_root(long long i)
{
    while (i > 5)
        i = scm_get_page_parent(i); // TODO: Constant time implementation.

    return i;
}

// Calculate the order (child index) of page i.

int scm_get_page_order(long long i)
{
    return (int) ((i - 6) - ((i - 6) / 4) * 4);
}

// Calculate the recursion level at which page i appears.

int scm_get_page_depth(long long i)
{
    return (int) ((log2i(i + 2) - 1) / 2);
}

// Calculate the breadth-first index of the parent of page i.

long long scm_get_page_parent(long long i)
{
    return (i - 6) / 4;
}

// Calculate the breadth-first index of the kth child of page p.

long long scm_get_page_child(long long p, int k)
{
    return 6 + 4 * p + k;
}

// Find the index of page (i, j) of (s, s) in the tree rooted at page p.

static long long scm_locate_page(long long p, long i, long j, long s)
{
    if (s > 1)
    {
        int k = 0;

        s >>= 1;

        if (i >= s) k |= 2;
        if (j >= s) k |= 1;

        return scm_locate_page(scm_get_page_child(p, k), i % s, j % s, s);
    }
    else return p;
}

// Find the indices of the four neighbors of page p.

void scm_get_page_neighbors(long long p, long long *u, long long *d,
                                         long long *r, long long *l)
{
    struct turn
    {
        unsigned int p :  3;
        unsigned int r :  3;
        unsigned int c :  3;
        unsigned int   : 23;
    };

    static const struct turn uu[6] = {
        { 2, 5, 3 }, { 2, 2, 0 }, { 5, 0, 5 },
        { 4, 3, 2 }, { 2, 3, 2 }, { 2, 0, 5 },
    };

    static const struct turn dd[6] = {
        { 3, 2, 3 }, { 3, 5, 0 }, { 4, 0, 2 },
        { 5, 3, 5 }, { 3, 0, 2 }, { 3, 3, 5 },
    };

    static const struct turn rr[6] = {
        { 4, 1, 3 }, { 5, 1, 3 }, { 1, 0, 1 },
        { 1, 3, 4 }, { 1, 1, 3 }, { 0, 1, 3 },
    };

    static const struct turn ll[6] = {
        { 5, 1, 0 }, { 4, 1, 0 }, { 0, 0, 4 },
        { 0, 3, 1 }, { 0, 1, 0 }, { 1, 1, 0 },
    };

    long o[6];
    long i = 0;
    long j = 0;
    long s;

    for (s = 1; p > 5; s <<= 1, p = scm_get_page_parent(p))
    {
        if (scm_get_page_order(p) & 1) j += s;
        if (scm_get_page_order(p) & 2) i += s;
    }

    o[0] = 0;
    o[1] = i;
    o[2] = j;
    o[3] = s     - 1;
    o[4] = s - i - 1;
    o[5] = s - j - 1;

    if (i     != 0) *u = scm_locate_page(p, i - 1, j, s);
    else            *u = scm_locate_page(uu[p].p, o[uu[p].r], o[uu[p].c], s);

    if (i + 1 != s) *d = scm_locate_page(p, i + 1, j, s);
    else            *d = scm_locate_page(dd[p].p, o[dd[p].r], o[dd[p].c], s);

    if (j     != 0) *r = scm_locate_page(p, i, j - 1, s);
    else            *r = scm_locate_page(rr[p].p, o[rr[p].r], o[rr[p].c], s);

    if (j + 1 != s) *l = scm_locate_page(p, i, j + 1, s);
    else            *l = scm_locate_page(ll[p].p, o[ll[p].r], o[ll[p].c], s);
}

//------------------------------------------------------------------------------

// Spherical cube map projection. The special sauce is Thousand Island dressing.

static void scube(int f, double x, double y, double *v)
{
    const double s = x * M_PI_2 - M_PI_4;
    const double t = y * M_PI_2 - M_PI_4;

    double w[3];

    w[0] =  sin(s) * cos(t);
    w[1] = -cos(s) * sin(t);
    w[2] =  cos(s) * cos(t);

    normalize(w);

    switch (f)
    {
        case 0: v[0] =  w[2]; v[1] =  w[1]; v[2] = -w[0]; break;
        case 1: v[0] = -w[2]; v[1] =  w[1]; v[2] =  w[0]; break;
        case 2: v[0] =  w[0]; v[1] =  w[2]; v[2] = -w[1]; break;
        case 3: v[0] =  w[0]; v[1] = -w[2]; v[2] =  w[1]; break;
        case 4: v[0] =  w[0]; v[1] =  w[1]; v[2] =  w[2]; break;
        case 5: v[0] = -w[0]; v[1] =  w[1]; v[2] = -w[2]; break;
    }
}

void scm_get_sample_corners(int f, long i, long j, long n, double *v)
{
    scube(f, (double) (j + 0) / n, (double) (i + 0) / n, v + 0);
    scube(f, (double) (j + 0) / n, (double) (i + 1) / n, v + 3);
    scube(f, (double) (j + 1) / n, (double) (i + 0) / n, v + 6);
    scube(f, (double) (j + 1) / n, (double) (i + 1) / n, v + 9);
}

void scm_get_sample_center(int f, long i, long j, long n, double *v)
{
    scube(f, (j + 0.5) / n, (i + 0.5) / n, v);
}

//------------------------------------------------------------------------------
