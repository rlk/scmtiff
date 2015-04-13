// SCMTIFF Copyright (C) 2012-2015 Robert Kooima
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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "scmdef.h"
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
        if (s->fp)
            fclose(s->fp);
        scm_free(s);
        free(s->name);
        free(s);
    }
}

// Open an SCM TIFF input file. Validate the header. Read and validate the first
// IFD. Initialize and return an SCM structure using the first IFD's parameters.

scm *scm_ifile(const char *name)
{
    scm *s = NULL;

    assert(name);

    if ((s = (scm *) calloc(sizeof (scm), 1)))
    {
        if ((s->fp = fopen(name, "r+b")))
        {
            if (scm_read_preamble(s))
            {
                if (scm_alloc(s))
                {
                    s->name = (char *) malloc(strlen(name) + 1);
                    strcpy(s->name, name);
                    return s;
                }
            }
        }
    }
    scm_close(s);
    return NULL;
}

// Open an SCM TIFF output file. Initialize and return an SCM structure with the
// given parameters. Write the TIFF header and SCM TIFF preface.

scm *scm_ofile(const char *name, int n, int c, int b, int g)
{
    scm *s = NULL;

    assert(name);
    assert(n > 0);
    assert(c > 0);
    assert(b > 0);

    if ((s = (scm *) calloc(sizeof (scm), 1)))
    {
        s->n =  n;
        s->c =  c;
        s->b =  b;
        s->g =  g;
        s->r = 16;

        if ((s->fp = fopen(name, "w+b")))
        {
            if (scm_write_preamble(s))
            {
                if (scm_alloc(s))
                {
                    if (scm_ffwd(s))
                    {
                        s->name = (char *) malloc(strlen(name) + 1);
                        strcpy(s->name, name);
                        return s;
                    }
                }
            }
        }
    }
    scm_close(s);
    return NULL;
}

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

void scm_get_sample_corners(int f, long i, long j, long n, double *v)
{
    scm_vector(f, (double) (i + 0) / n, (double) (j + 0) / n, v + 0);
    scm_vector(f, (double) (i + 1) / n, (double) (j + 0) / n, v + 3);
    scm_vector(f, (double) (i + 0) / n, (double) (j + 1) / n, v + 6);
    scm_vector(f, (double) (i + 1) / n, (double) (j + 1) / n, v + 9);
}

void scm_get_sample_center(int f, long i, long j, long n, double *v)
{
    scm_vector(f, (i + 0.5) / n, (j + 0.5) / n, v);
}

//------------------------------------------------------------------------------

// Standard-library-compatible long long compare function for bsearch and qsort.

static int llcompare(const void *p, const void *q)
{
    const long long *a = (const long long *) p;
    const long long *b = (const long long *) q;

    if      (a[0] < b[0]) return -1;
    else if (a[0] > b[0]) return +1;
    else                  return  0;
}

// Adapt the standard library bsearch to an array of long longs. Return the
// index of key x in array v of length c

static long long llsearch(long long x, long long c, const long long *v)
{
    void  *p = bsearch(&x, v, (size_t) c, sizeof (long long), llcompare);
    return p ? (long long *) p - v : -1;
}

// Page x is a leaf if none of its children appear in the array.

static bool isleaf(long long x, long long c, const long long *v)
{
    if      (llsearch(scm_page_child(x, 0), c, v) >= 0) return false;
    else if (llsearch(scm_page_child(x, 1), c, v) >= 0) return false;
    else if (llsearch(scm_page_child(x, 2), c, v) >= 0) return false;
    else if (llsearch(scm_page_child(x, 3), c, v) >= 0) return false;

    else return true;
}

// Allocate and initialize a sorted array with the indices of all present pages.

static long long scm_scan_indices(scm *s, long long **v)
{
    ifd d;

    long long n = 0;
    long long c = 0;
    long long o;

    // Scan the file to count the pages.

    for (o = scm_rewind(s); scm_read_ifd(s, &d, o); o = (long long) d.next)
        n++;

    // Allocate storage for all indices.

    if ((v[0] = (long long *) calloc((size_t) n, sizeof (long long))) == NULL)
        return 0;

    // Scan the file to read the indices.

    for (o = scm_rewind(s); scm_read_ifd(s, &d, o); o = (long long) d.next)
        v[0][c++] = (long long) d.page_number.offset;

    // Sort the array.

    qsort(v[0], (size_t) c, sizeof (long long), llcompare);

    return c;
}

// Allocate and initialize an array giving the file offsets of all present pages
// in index-sorted order. O(n log n).

static long long scm_scan_offsets(scm *s, long long **v,
                      long long xc, const long long *xv)
{
    ifd d;

    long long o;
    long long i;

    // Allocate storage for all offsets.

    if ((v[0] = (long long *) calloc((size_t) xc, sizeof (long long))) == NULL)
        return 0;

    // Scan the file to read the offsets.

    for (o = scm_rewind(s); scm_read_ifd(s, &d, o); o = (long long) d.next)
        if ((i = llsearch((long long) d.page_number.offset, xc, xv)) >= 0)
            v[0][i] = o;

    return xc;
}

// Append page x to the index array v of length c. Recursively add all children
// to depth d. Return the new array length.

static long long scm_grow_leaf(long long x, long long **v, long long c, int d)
{
    v[0][c++] = x;

    if (d > 0)
    {
        c = scm_grow_leaf(scm_page_child(x, 0), v, c, d - 1);
        c = scm_grow_leaf(scm_page_child(x, 1), v, c, d - 1);
        c = scm_grow_leaf(scm_page_child(x, 2), v, c, d - 1);
        c = scm_grow_leaf(scm_page_child(x, 3), v, c, d - 1);
    }
    return c;
}

// Given index array xv with length xc, extend the page hierarchy adding d
// levels to each leaf. Allocate a new index array to receive its indices.

static long long scm_grow_leaves(scm *s, long long **v,
                     long long xc, const long long *xv, int d)
{
    // Count the number of pages in the extended catalog.

    long long m = xc;
    long long c = xc;
    long long i;

    for (i = 0; i < xc; ++i)
        if (isleaf(xv[i], xc, xv))
            m += scm_page_count(d);

    // Allocate storage for the extended catalog.

    if ((v[0] = (long long *) malloc((size_t) m * sizeof (long long))) == NULL)
        return 0;

    // Copy the basic catalog.

    memcpy(v[0], xv, (size_t) xc * sizeof (long long));

    // Extend the catalog.

    for (i = 0; d && i < xc; ++i)
        if (isleaf(xv[i], xc, xv))
        {
            c = scm_grow_leaf(scm_page_child(xv[i], 0), v, c, d - 1);
            c = scm_grow_leaf(scm_page_child(xv[i], 1), v, c, d - 1);
            c = scm_grow_leaf(scm_page_child(xv[i], 2), v, c, d - 1);
            c = scm_grow_leaf(scm_page_child(xv[i], 3), v, c, d - 1);
        }

    // Sort the extended catalog.

    qsort(v[0], (size_t) c, sizeof (long long), llcompare);

    return c;
}

// Compute the extrema of an internal node. This is trivially defined in terms
// of the extrema of its children.

static void scm_bound_node(scm *s, long long x,
                                   long long c, const long long *v, float *av,
                                                                    float *zv)
{
    long long i  = llsearch(x, c, v);

    long long i0 = llsearch(scm_page_child(x, 0), c, v);
    long long i1 = llsearch(scm_page_child(x, 1), c, v);
    long long i2 = llsearch(scm_page_child(x, 2), c, v);
    long long i3 = llsearch(scm_page_child(x, 3), c, v);

    for (int k = 0; k < s->c; ++k)
    {
        const long long di = i * s->c + k;

        av[di] =  FLT_MAX;
        zv[di] = -FLT_MAX;

        if (i0 >= 0)
        {
            const long long si = i0 * s->c + k;
            if (av[di] > av[si]) av[di] = av[si];
            if (zv[di] < zv[si]) zv[di] = zv[si];
        }
        if (i1 >= 0)
        {
            const long long si = i1 * s->c + k;
            if (av[di] > av[si]) av[di] = av[si];
            if (zv[di] < zv[si]) zv[di] = zv[si];
        }
        if (i2 >= 0)
        {
            const long long si = i2 * s->c + k;
            if (av[di] > av[si]) av[di] = av[si];
            if (zv[di] < zv[si]) zv[di] = zv[si];
        }
        if (i3 >= 0)
        {
            const long long si = i3 * s->c + k;
            if (av[di] > av[si]) av[di] = av[si];
            if (zv[di] < zv[si]) zv[di] = zv[si];
        }
    }
}

// Recursively subdivide a leaf node to a depth of d. Determine the extrema of
// each new leaf by scanning the pixel buffer pp.

static void scm_bound_leaf(scm *s, long long x,  const float     *pp,
                                   long long yc, const long long *yv,
                                   float *av,
                                   float *zv,
                                   int l, int r, int t, int b, int d)
{
    long long i;

    // Subdivide as far as needed.

    if (d > 0)
    {
        long long x0 = scm_page_child(x, 0);
        long long x1 = scm_page_child(x, 1);
        long long x2 = scm_page_child(x, 2);
        long long x3 = scm_page_child(x, 3);

        int h = (l + r) / 2;
        int v = (b + t) / 2;

        scm_bound_leaf(s, x0, pp, yc, yv, av, zv, l, h, t, v, d - 1);
        scm_bound_leaf(s, x1, pp, yc, yv, av, zv, h, r, t, v, d - 1);
        scm_bound_leaf(s, x2, pp, yc, yv, av, zv, l, h, v, b, d - 1);
        scm_bound_leaf(s, x3, pp, yc, yv, av, zv, h, r, v, b, d - 1);

        scm_bound_node(s, x, yc, yv, av, zv);
    }

    // Sample the leaf subdivision and note the extrema.

    else if ((i = llsearch(x, yc, yv)) >= 0)
    {
        for (int k = 0; k < s->c; ++k)
        {
            const long long di = i * s->c + k;

            av[di] =  FLT_MAX;
            zv[di] = -FLT_MAX;

            for     (int y = t; y < b; ++y)
                for (int x = l; x < r; ++x)
                {
                    const int si = ((y + 1) * (s->n + 2) + (x + 1)) * s->c + k;

                    if (av[di] > pp[si]) av[di] = pp[si];
                    if (zv[di] < pp[si]) zv[di] = pp[si];
                }
        }
    }
}

// Compute the min and max values of all pages. ov gives the file offset of all
// real pages and xv gives the page index of all real pages. yv gives the page
// index of all pages, real or virtual, and d gives the subdivision depth of
// virtual pages. Allocate and initialize minv and maxv with the min and max
// values of all real and virtual pages.

static bool scm_bound(scm *s, long long xc, const long long *xv,
                              long long yc, const long long *yv,
                              long long oc, const long long *ov,
                                                    void **minv,
                                                    void **maxv, int d)
{
    const size_t sz = tifsizeof(scm_type(s));
    const size_t yz = (size_t) yc * (size_t) s->c;

    float *pp = NULL;
    float *av = NULL;
    float *zv = NULL;

    bool st = false;

    if ((pp = scm_alloc_buffer(s)))
    {
        if ((av = (float *) malloc(yz * sizeof (float))) &&
            (zv = (float *) malloc(yz * sizeof (float))))
        {
            // Calculate bounds for all pages.

            for (long long i = xc - 1; i >= 0; --i)
            {
                if (isleaf(xv[i], xc, xv))
                {
                    if (scm_read_page (s, ov[i], pp))
                        scm_bound_leaf(s, xv[i], pp,
                                       yc, yv, av, zv, 0, s->n, 0, s->n, d);
                }
                else scm_bound_node(s, xv[i], yc, yv, av, zv);
            }

            // Convert floating point values to the SCM value type.

            if ((minv[0] = malloc(yz * sz)) &&
                (maxv[0] = malloc(yz * sz)))
            {
                ftob(minv[0], av, yz, s->b, s->g);
                ftob(maxv[0], zv, yz, s->b, s->g);

                st = true;
            }
            free(zv);
            free(av);
        }
        free(pp);
    }
    return st;
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

    long long o;
    uint64_t oo;
    uint64_t lo;
    uint16_t sc;

    ifd d;

    if (scm_init_ifd(s, &d))
    {
        if (scm_ffwd(s))
        {
            if ((o = scm_write_ifd(s, &d, 0)) >= 0)
            {
                if ((scm_write_data(s, f, &oo, &lo, &sc)))
                {
                    if (scm_align(s) >= 0)
                    {
                        uint64_t rr = (uint64_t) s->r;
                        uint64_t xx = (uint64_t) x;

                        scm_field(&d.strip_offsets,     0x0111, 16, sc, oo);
                        scm_field(&d.rows_per_strip,    0x0116,  3,  1, rr);
                        scm_field(&d.strip_byte_counts, 0x0117,  4, sc, lo);
                        scm_field(&d.page_number,       0x0129,  4,  1, xx);

                        if (scm_write_ifd(s, &d, o) >= 1)
                        {
                            if (scm_link_list(s, o, b))
                            {
                                if (scm_ffwd(s))
                                {
                                    fflush(s->fp);
                                    return o;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
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

    ifd d;

    if (scm_read_ifd(t, &d, o))
    {
        uint64_t oo = (uint64_t) d.strip_offsets.offset;
        uint64_t lo = (uint64_t) d.strip_byte_counts.offset;
        uint16_t sc = (uint16_t) d.strip_byte_counts.count;

        uint64_t O[256];
        uint32_t L[256];

        d.next = 0;

        if (scm_read_zips(t, t->zipv, oo, lo, sc, O, L))
        {
            if ((o = scm_write_ifd(s, &d, 0)) >= 0)
            {
                if (scm_write_zips(s, t->zipv, &oo, &lo, &sc, O, L))
                {
                    if (scm_align(s) >= 0)
                    {
                        uint64_t rr = (uint64_t) s->r;

                        scm_field(&d.strip_offsets,     0x0111, 16, sc, oo);
                        scm_field(&d.rows_per_strip,    0x0116,  3,  1, rr);
                        scm_field(&d.strip_byte_counts, 0x0117,  4, sc, lo);

                        if (scm_write_ifd(s, &d, o) >= 0)
                        {
                            if (scm_link_list(s, o, b))
                            {
                                if (scm_ffwd(s))
                                {
                                    fflush(s->fp);
                                    return o;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

// Move the SCM TIFF file pointer to the first IFD and return its offset.

long long scm_rewind(scm *s)
{
    header h;
    hfd    d;

    assert(s);

    if (scm_read_header(s, &h))
    {
        if (scm_read_hfd(s, &d, h.first_ifd))
        {
            if (scm_seek(s, d.next))
            {
                return (long long) d.next;
            }
        }
    }
    return 0;
}

// Calculate and write all metadata to SCM s.

bool scm_finish(scm *s, const char *txt, int d)
{
    assert(s);

    const size_t sz = tifsizeof(scm_type(s));

    long long  xc =    0;
    long long *xv = NULL;
    long long  yc =    0;
    long long *yv = NULL;
    long long  oc =    0;
    long long *ov = NULL;
    void    *minv = NULL;
    void    *maxv = NULL;

    bool st = false;

    // Allocate and initialize buffers for all metadata data.

    if ((xc = scm_scan_indices(s, &xv)))
    {
        if ((yc = scm_grow_leaves(s, &yv, xc, xv, d)))
        {
            if ((oc = scm_scan_offsets(s, &ov, yc, yv)))
            {
                if (scm_bound(s, xc, xv, yc, yv, oc, ov, &minv, &maxv, d))
                {
                    // Append all metadata.

                    uint16_t t  = (uint16_t) scm_type(s);
                    uint64_t bc = (uint64_t) (yc * s->c);
                    uint64_t tc = (uint64_t) strlen(txt) + 1;
                    uint64_t yo = 0;
                    uint64_t oo = 0;
                    uint64_t ao = 0;
                    uint64_t zo = 0;
                    uint64_t to = 0;

                    if (scm_ffwd(s))
                    {
                        yo = scm_write(s,   yv, (size_t) yc * sizeof (long long));
                        oo = scm_write(s,   ov, (size_t) oc * sizeof (long long));
                        ao = scm_write(s, minv, (size_t) bc * sz);
                        zo = scm_write(s, maxv, (size_t) bc * sz);
                        to = scm_write(s,  txt, (size_t) tc);
                    }

                    // Update the header file directory.

                    header h;
                    hfd    d;

                    if (scm_read_header(s, &h))
                    {
                        if (scm_read_hfd(s, &d, h.first_ifd))
                        {
                            scm_field(&d.page_index,   SCM_PAGE_INDEX,  16, yc, yo);
                            scm_field(&d.page_offset,  SCM_PAGE_OFFSET, 16, oc, oo);
                            scm_field(&d.page_minimum, SCM_PAGE_MINIMUM, t, bc, ao);
                            scm_field(&d.page_maximum, SCM_PAGE_MAXIMUM, t, bc, zo);
                            scm_field(&d.description,  0x010E,           2, tc, to);

                            st = (scm_write_hfd(s, &d, h.first_ifd) > 0);
                        }
                    }
                }
            }
        }
    }

    free(maxv);
    free(minv);
    free(yv);
    free(ov);
    free(xv);

    return st;
}

// LibTIFF doesn't allow a page with a non-zero size to have no data given.
// As a cheap hack, point the header fields at the first page's data.

bool scm_polish(scm *s)
{
    assert(s);

    bool st = false;

    header h;
    hfd    d;
    ifd    i;

    if (scm_read_header(s, &h))
    {
        if (scm_read_hfd(s, &d, h.first_ifd))
        {
            if (scm_read_ifd(s, &i, d.next))
            {
                d.strip_offsets     = i.strip_offsets;
                d.strip_byte_counts = i.strip_byte_counts;
            }
            st = (scm_write_hfd(s, &d, h.first_ifd) > 0);
        }
    }
    return st;
}

//------------------------------------------------------------------------------

// Read the SCM TIFF IFD at offset o. Assume p provides space for one page of
// data to be stored.

bool scm_read_page(scm *s, long long o, float *p)
{
    ifd i;

    assert(s);

    if (scm_read_ifd(s, &i, o))
    {
        uint64_t oo = (uint64_t) i.strip_offsets.offset;
        uint64_t lo = (uint64_t) i.strip_byte_counts.offset;
        uint16_t sc = (uint16_t) i.strip_byte_counts.count;

        return scm_read_data(s, p, oo, lo, sc);
    }
    else apperr("Failed to read SCM TIFF IFD from %s", s->name);

    return false;
}

//------------------------------------------------------------------------------

// Scan the file and catalog the index and offset of all pages.

bool scm_scan_catalog(scm *s)
{
    assert(s);

    // Release any existing catalog buffers.

    if (s->xv) free(s->xv);
    if (s->ov) free(s->ov);

    // Scan the indices and offsets.

    if ((s->xc = scm_scan_indices(s, &s->xv)))
    {
        if ((s->oc = scm_scan_offsets(s, &s->ov, s->xc, s->xv)))
        {
            return true;
        }
    }
    return false;
}

// Return the number of catalog entries.

long long scm_get_length(scm *s)
{
    assert(s);
    return s->xc;
}

// Return the index of the i'th catalog entry.

long long scm_get_index(scm *s, long long i)
{
    assert(s);
    assert(s->xc);
    assert(s->xv);
    assert(0 <= i && i < s->xc);
    return s->xv[i];
}

// Return the offset of the i'th catalog entry.

long long scm_get_offset(scm *s, long long i)
{
    assert(s);
    assert(s->oc);
    assert(s->ov);
    assert(0 <= i && i < s->oc);
    return s->ov[i];
}

// Search for the catalog entry of a given page index.

long long scm_search(scm *s, long long x)
{
    assert(s);
    assert(s->xc);
    assert(s->xv);

    if (x < s->xv[        0]) return -1;
    if (x > s->xv[s->xc - 1]) return -1;

    return llsearch(x, s->xc, s->xv);
}

//------------------------------------------------------------------------------
