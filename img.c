// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>

#include "img.h"
#include "err.h"
#include "util.h"

//------------------------------------------------------------------------------
// Access a raw image buffer. Convert from the image's internal format to double
// precision floating point. Assume the return buffer c has the same or greater
// channel count as the image. Expect out-of-bounds references to be made in the
// normal process of linear-filtered multisampling, and return zero if necessary.

static int get8u(img *p, int i, int j, double *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        unsigned char *q = (unsigned char *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = q[3] / 255.0;
            case 3: c[2] = q[2] / 255.0;
            case 2: c[1] = q[1] / 255.0;
            case 1: c[0] = q[0] / 255.0;
        }
        return 1;
    }

    memset(c, 0, p->c * sizeof (double));
    return 0;
}

static int get8s(img *p, int i, int j, double *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        char *q = (char *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = q[3] / 127.0;
            case 3: c[2] = q[2] / 127.0;
            case 2: c[1] = q[1] / 127.0;
            case 1: c[0] = q[0] / 127.0;
        }
        return 1;
    }

    memset(c, 0, p->c * sizeof (double));
    return 0;
}

static int get16u(img *p, int i, int j, double *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        unsigned short *q = (unsigned short *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = q[3] / 65535.0;
            case 3: c[2] = q[2] / 65535.0;
            case 2: c[1] = q[1] / 65535.0;
            case 1: c[0] = q[0] / 65535.0;
        }
        return 1;
    }

    memset(c, 0, p->c * sizeof (double));
    return 0;
}

static int get16s(img *p, int i, int j, double *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        short *q = (short *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = q[3] / 32767.0;
            case 3: c[2] = q[2] / 32767.0;
            case 2: c[1] = q[1] / 32767.0;
            case 1: c[0] = q[0] / 32767.0;
        }
        return 1;
    }

    memset(c, 0, p->c * sizeof (double));
    return 0;
}

//------------------------------------------------------------------------------

// Allocate, initialize, and return an image structure representing a pixel
// buffer with width w, height h, channel count c, bits-per-channel count b,
// and signedness s.

img *img_alloc(int w, int h, int c, int b, int s)
{
    size_t n = (size_t) w * (size_t) h * (size_t) c * (size_t) b / 8;
    img   *p = NULL;

    if ((p = (img *) calloc(1, sizeof (img))))
    {
        if ((p->p = malloc(n)))
        {
            p->sample = img_sample_spheremap;
            p->n = n;
            p->w = w;
            p->h = h;
            p->c = c;
            p->b = b;
            p->s = s;
            p->d = 0;

            if      (b ==  8 && s == 0) p->get = get8u;
            else if (b ==  8 && s == 1) p->get = get8s;
            else if (b == 16 && s == 0) p->get = get16u;
            else if (b == 16 && s == 1) p->get = get16s;

            return p;
        }
        else apperr("Failed to allocate image buffer");
    }
    else apperr("Failed to allocate image structure");

    img_close(p);
    return NULL;
}

img *img_mmap(int w, int h, int c, int b, int s, const char *name, off_t o)
{
    size_t n = (size_t) w * (size_t) h * (size_t) c * (size_t) b / 8;
    img   *p = NULL;
    int    d = 0;

    if ((p = (img *) calloc(1, sizeof (img))))
    {
        if ((d = open(name, O_RDONLY)) != -1)
        {
            if ((p->p = mmap(0, n, PROT_READ, MAP_PRIVATE, d, o)) != MAP_FAILED)
            {
                p->sample = img_sample_spheremap;
                p->n = n;
                p->w = w;
                p->h = h;
                p->c = c;
                p->b = b;
                p->s = s;
                p->d = d;

                if      (b ==  8 && s == 0) p->get = get8u;
                else if (b ==  8 && s == 1) p->get = get8s;
                else if (b == 16 && s == 0) p->get = get16u;
                else if (b == 16 && s == 1) p->get = get16s;

                return p;
            }
            else syserr("Failed to mmap %s", name);
        }
        else syserr("Failed to open %s", name);
    }
    else apperr("Failed to allocate image structure");

    img_close(p);
    return NULL;
}

void img_close(img *p)
{
    if (p)
    {
        if (p->d)
        {
            munmap(p->p, p->n);
            close(p->d);
        }
        else
        {
            free(p->p);
        }
        free(p);
    }
}

// Calculate and return a pointer to scanline r of the given image. This is
// useful during image I/O.

void *img_scanline(img *p, int r)
{
    assert(p);
    return (char *) p->p + ((size_t) p->w *
                            (size_t) p->c *
                            (size_t) p->b *
                            (size_t) r / 8);
}

//------------------------------------------------------------------------------

// Perform a linearly-filtered sampling of the image p. The filter position
// i, j is smoothly-varying in the range [0, w), [0, h).

int img_linear(img *p, double i, double j, double *c)
{
    const double s = i - floor(i);
    const double t = j - floor(j);

    const int ia = (int) floor(i);
    const int ib = (int)  ceil(i);
    const int ja = (int) floor(j);
    const int jb = (int)  ceil(j);

    double aa[4];
    double ab[4];
    double ba[4];
    double bb[4];

    int k = 0;

    k += p->get(p, ia, ja, aa);
    k += p->get(p, ia, jb, ab);
    k += p->get(p, ib, ja, ba);
    k += p->get(p, ib, jb, bb);

    if (k)
        switch (p->c)
        {
            case 4: c[3] = lerp2(aa[3], ab[3], ba[3], bb[3], s, t);
            case 3: c[2] = lerp2(aa[2], ab[2], ba[2], bb[2], s, t);
            case 2: c[1] = lerp2(aa[1], ab[1], ba[1], bb[1], s, t);
            case 1: c[0] = lerp2(aa[0], ab[0], ba[0], bb[0], s, t);
        }

    return k;
}

//------------------------------------------------------------------------------

// Perform a spherically-projected linearly-filtered sampling of image p.

int img_sample_spheremap(img *p, const double *v, double *c)
{
    const double lon = atan2(v[0], -v[2]), lat = asin(v[1]);

    const double j = (p->w    ) * 0.5 * (M_PI   + lon) / M_PI;
    const double i = (p->h - 1) * 0.5 * (M_PI_2 - lat) / M_PI_2;

    return img_linear(p, i, j, c);
}

int img_sample_test(img *p, const double *v, double *c)
{
    switch (p->c)
    {
        case 4: c[3] =                1.0;
        case 3: c[2] = (v[2] + 1.0) / 2.0;
        case 2: c[1] = (v[1] + 1.0) / 2.0;
        case 1: c[0] = (v[0] + 1.0) / 2.0;
    }
    return 1;
}

//------------------------------------------------------------------------------
