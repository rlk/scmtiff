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

off_t scm_append(scm *s, off_t b, int x, const float *f)
{
    assert(s);
    assert(f);

    ifd   D = s->D;
    off_t o;

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
                    uint64_t os = (uint64_t) (o + offsetof(ifd, sub));

                    set_field(&D.strip_offsets,     0x0111, 16, sc, oo);
                    set_field(&D.rows_per_strip,    0x0116,  3,  1, s->r);
                    set_field(&D.strip_byte_counts, 0x0117,  4, sc, lo);
                    set_field(&D.sub_ifds,          0x014A, 18,  4, os);
                    set_field(&D.page_index,        0xFFB1,  4,  1,  x);

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

off_t scm_repeat(scm *s, off_t b, scm *t, off_t o)
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
        uint64_t oo = D.strip_offsets.offset;
        uint64_t lo = D.strip_byte_counts.offset;
        uint16_t sc = D.strip_byte_counts.count;

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
                            uint64_t os = (uint64_t) (o + offsetof(ifd, sub));

                            set_field(&D.strip_offsets,     0x0111, 16, sc, oo);
                            set_field(&D.rows_per_strip,    0x0116,  3,  1, s->r);
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

// Scan the offset and page index of each IFD and rewrite all SubIFD fields.
// This has the effect of making the data structure depth-first seek-able
// without the need for a cataloging and mapping pre-pass. We may safely
// assume that this process does not change the length of the file or any
// offsets within it.

void scm_relink(scm *s)
{
    off_t *m;
    int    d;
    ifd    i;

    assert(s);

    if ((d = scm_mapping(s, &m)))
    {
        for (int x = 0; x < scm_get_page_count(d); ++x)
            if (m[x] && scm_read_ifd(s, &i, m[x]) == 1)
            {
                i.sub[0] = m[scm_get_page_child(x, 0)];
                i.sub[1] = m[scm_get_page_child(x, 1)];
                i.sub[2] = m[scm_get_page_child(x, 2)];
                i.sub[3] = m[scm_get_page_child(x, 3)];

                scm_write_ifd(s, &i, m[x]);
            }

        free(m);
    }
}

//------------------------------------------------------------------------------

// Read the SCM TIFF IFD at offset o. Assume p provides space for one page of
// data to be stored.

size_t scm_read_page(scm *s, off_t o, float *p)
{
    ifd i;

    assert(s);

    if (scm_read_ifd(s, &i, o) == 1)
    {
        uint64_t oo = (size_t) i.strip_offsets.offset;
        uint64_t lo = (size_t) i.strip_byte_counts.offset;
        uint16_t sc = (size_t) i.strip_byte_counts.count;

        return scm_read_data(s, p, oo, lo, sc);
    }
    else apperr("Failed to read SCM TIFF IFD");

    return 0;
}

// Read the SCM TIFF IFD at offset o. Return this IFD's page index. If n is not
// null, store the next IFD offset there. If v is not null, assume it can store
// four offsets and copy the SubIFDs there.

int scm_read_node(scm *s, off_t o, off_t *n, off_t *v)
{
    ifd i;

    assert(s);

    if (o)
    {
        if (scm_read_ifd(s, &i, o) == 1)
        {
            if (n)
                n[0] = (off_t) i.next;
            if (v)
            {
                v[0] = (off_t) i.sub[0];
                v[1] = (off_t) i.sub[1];
                v[2] = (off_t) i.sub[2];
                v[3] = (off_t) i.sub[3];
            }
            return (int) i.page_index.offset;
        }
        else apperr("Failed to read SCM TIFF IFD");
    }
    return -1;
}

// Determine the file offset and page index of each IFD in SCM s. Return these
// in a newly-allocated array of offsets, indexed by page index. The allocated
// length of this array is sufficient to store a full tree, regardless of the
// true sparseness of SCM s.

int scm_mapping(scm *s, off_t **mv)
{
    int l = 0;
    int x = 0;

    off_t o;
    off_t n;

    assert(s);

    // Determine the largest page index.

    for (o = scm_rewind(s); (x = scm_read_node(s, o, &n, 0)) >= 0; o = n)
        if (l < x)
            l = x;

    // Determine the index and offset of each page and initialize a mapping.

    int d = scm_get_page_depth(l);
    int m = scm_get_page_count(d);

    if ((*mv = (off_t *) calloc(m, sizeof (off_t))))
    {
        for (o = scm_rewind(s); (x = scm_read_node(s, o, &n, 0)) >= 0; o = n)
            (*mv)[x] = o;

        return d;
    }
    return -1;
}

//------------------------------------------------------------------------------

// Allocate and return a buffer with the proper size to fit one page of data,
// as determined by the parameters of SCM s, assuming float precision samples.

float *scm_alloc_buffer(scm *s)
{
    return (float *) malloc((s->n + 2) * (s->n + 2) * s->c * sizeof (float));
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

int log2i(int n)
{
    int r = 0;

    if (n >= (1 << 16)) { n >>= 16; r += 16; }
    if (n >= (1 <<  8)) { n >>=  8; r +=  8; }
    if (n >= (1 <<  4)) { n >>=  4; r +=  4; }
    if (n >= (1 <<  2)) { n >>=  2; r +=  2; }
    if (n >= (1 <<  1)) {           r +=  1; }

    return r;
}

// Traverse up the tree to find the root page of page i.

int scm_get_page_root(int i)
{
    while (i > 5)
        i = scm_get_page_parent(i);

    return i;
}

// Calculate the recursion level at which page i appears.

int scm_get_page_depth(int i)
{
    return (log2i(i + 2) - 1) / 2;
}

// Calculate the number of pages in an SCM of depth d.

int scm_get_page_count(int d)
{
    return (1 << (2 * d + 3)) - 2;
}

// Calculate the breadth-first index of the ith child of page p.

int scm_get_page_child(int p, int i)
{
    return 6 + 4 * p + i;
}

// Calculate the breadth-first index of the parent of page i.

int scm_get_page_parent(int i)
{
    return (i - 6) / 4;
}

// Calculate the order (child index) of page i.

int scm_get_page_order(int i)
{
    return (i - 6) - ((i - 6) / 4) * 4;
}

// Find the index of page (i, j) of (s, s) in the tree rooted at page p.

int scm_locate_page(int p, int i, int j, int s)
{
    if (s > 1)
    {
        int c = 0;

        s >>= 1;

        if (i >= s) c |= 2;
        if (j >= s) c |= 1;

        return scm_locate_page(scm_get_page_child(p, c), i % s, j % s, s);
    }
    else return p;
}

// Find the indices of the four neighbors of page p.

void scm_get_page_neighbors(int p, int *u, int *d, int *r, int *l)
{
    struct turn
    {
        unsigned int p : 3;
        unsigned int r : 3;
        unsigned int c : 3;
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

    int o[6];
    int i = 0;
    int j = 0;
    int s;

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

#define NORM3 0.57735027f

static const float page_v[8][3] = {
    {  NORM3,  NORM3,  NORM3 },
    { -NORM3,  NORM3,  NORM3 },
    {  NORM3, -NORM3,  NORM3 },
    { -NORM3, -NORM3,  NORM3 },
    {  NORM3,  NORM3, -NORM3 },
    { -NORM3,  NORM3, -NORM3 },
    {  NORM3, -NORM3, -NORM3 },
    { -NORM3, -NORM3, -NORM3 },
};

static const int page_i[6][4] = {
    { 0, 4, 2, 6 },
    { 5, 1, 7, 3 },
    { 5, 4, 1, 0 },
    { 3, 2, 7, 6 },
    { 1, 0, 3, 2 },
    { 4, 5, 6, 7 },
};

static void _get_page_corners(int i, int d, const float *A,
                                            const float *B,
                                            const float *C,
                                            const float *D, float *p)
{
    memcpy(p + i * 12 + 0, A, 3 * sizeof (float));
    memcpy(p + i * 12 + 3, B, 3 * sizeof (float));
    memcpy(p + i * 12 + 6, C, 3 * sizeof (float));
    memcpy(p + i * 12 + 9, D, 3 * sizeof (float));

    if (d)
    {
        float N[3];
        float S[3];
        float W[3];
        float E[3];
        float M[3];

        mid2(N, A, B);
        mid2(S, C, D);
        mid2(W, A, C);
        mid2(E, B, D);
        mid4(M, A, B, C, D);

        _get_page_corners(scm_get_page_child(i, 0), d - 1, A, N, W, M, p);
        _get_page_corners(scm_get_page_child(i, 1), d - 1, N, B, M, E, p);
        _get_page_corners(scm_get_page_child(i, 2), d - 1, W, M, C, S, p);
        _get_page_corners(scm_get_page_child(i, 3), d - 1, M, E, S, D, p);
    }
}

// Recursively compute the corner vectors of all pages down to depth d. Assume
// array p provides storage for scm_get_page_count(d) pages, where each page
// is four 3D vectors (12 floats).

void scm_get_page_corners(int d, float *p)
{
    int i;

    for (i = 0; i < 6; ++i)
        _get_page_corners(i, d, page_v[page_i[i][0]],
                                page_v[page_i[i][1]],
                                page_v[page_i[i][2]],
                                page_v[page_i[i][3]], p);
}

//------------------------------------------------------------------------------

void scm_get_samp_corners(const float *u, int i, int j, int n, float *v)
{
    const float i0 = (float) (i + 0) / n;
    const float i1 = (float) (i + 1) / n;
    const float j0 = (float) (j + 0) / n;
    const float j1 = (float) (j + 1) / n;

    slerp2(v + 0, u + 0, u + 3, u + 6, u + 9, j0, i0);
    slerp2(v + 3, u + 0, u + 3, u + 6, u + 9, j1, i0);
    slerp2(v + 6, u + 0, u + 3, u + 6, u + 9, j0, i1);
    slerp2(v + 9, u + 0, u + 3, u + 6, u + 9, j1, i1);

    normalize(v + 0);
    normalize(v + 3);
    normalize(v + 6);
    normalize(v + 9);
}

void scm_get_samp_vector(const float *u, int i, int j, int n, float *v)
{
    const float ic = (float) (i + 0.5f) / n;
    const float jc = (float) (j + 0.5f) / n;

    slerp2(v, u + 0, u + 3, u + 6, u + 9, jc, ic);

    normalize(v);
}

//------------------------------------------------------------------------------
