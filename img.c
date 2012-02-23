// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

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

// Detect PDS saturation codes.

static float cleanf(float f)
{
    unsigned int *w = (unsigned int *) (&f);

    if (*w == 0xFF7FFFFB) return 0.0;  // Core null
    if (*w == 0xFF7FFFFC) return 0.0;  // Representation saturation low
    if (*w == 0xFF7FFFFD) return 0.0;  // Instrumentation saturation low
    if (*w == 0xFF7FFFFE) return 1.0;  // Representation saturation high
    if (*w == 0xFF7FFFFF) return 1.0;  // Instrumentation saturation high

    if (isnormal(f))
        return (float) f;
    else
        return 0.0;
}

//------------------------------------------------------------------------------
// Access a raw image buffer. Convert from the image's internal format to float
// precision floating point. Assume the return buffer c has the same or greater
// channel count as the image. Expect out-of-bounds references to be made in the
// normal process of linear-filtered multisampling, and return zero if necessary.

static int get8u(img *p, int i, int j, float *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        unsigned char *q = (unsigned char *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = ((float) q[3] - p->dnorm) * p->knorm;
            case 3: c[2] = ((float) q[2] - p->dnorm) * p->knorm;
            case 2: c[1] = ((float) q[1] - p->dnorm) * p->knorm;
            case 1: c[0] = ((float) q[0] - p->dnorm) * p->knorm;
        }
        return 1;
    }
    memset(c, 0, p->c * sizeof (float));
    return 0;
}

static int get8s(img *p, int i, int j, float *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        char *q = (char *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = ((float) q[3] - p->dnorm) * p->knorm;
            case 3: c[2] = ((float) q[2] - p->dnorm) * p->knorm;
            case 2: c[1] = ((float) q[1] - p->dnorm) * p->knorm;
            case 1: c[0] = ((float) q[0] - p->dnorm) * p->knorm;
        }
        return 1;
    }
    memset(c, 0, p->c * sizeof (float));
    return 0;
}

static int get16u(img *p, int i, int j, float *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        unsigned short *q = (unsigned short *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = ((float) q[3] - p->dnorm) * p->knorm;
            case 3: c[2] = ((float) q[2] - p->dnorm) * p->knorm;
            case 2: c[1] = ((float) q[1] - p->dnorm) * p->knorm;
            case 1: c[0] = ((float) q[0] - p->dnorm) * p->knorm;
        }
        return 1;
    }
    memset(c, 0, p->c * sizeof (float));
    return 0;
}

static int get16s(img *p, int i, int j, float *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        short *q = (short *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = ((float) q[3] - p->dnorm) * p->knorm;
            case 3: c[2] = ((float) q[2] - p->dnorm) * p->knorm;
            case 2: c[1] = ((float) q[1] - p->dnorm) * p->knorm;
            case 1: c[0] = ((float) q[0] - p->dnorm) * p->knorm;
        }
        return 1;
    }
    memset(c, 0, p->c * sizeof (float));
    return 0;
}

static int get32f(img *p, int i, int j, float *c)
{
    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        float *q = (float *) p->p + p->c * (p->w * i + j);

        switch (p->c)
        {
            case 4: c[3] = (cleanf(q[3]) - p->dnorm) * p->knorm;
            case 3: c[2] = (cleanf(q[2]) - p->dnorm) * p->knorm;
            case 2: c[1] = (cleanf(q[1]) - p->dnorm) * p->knorm;
            case 1: c[0] = (cleanf(q[0]) - p->dnorm) * p->knorm;
        }
        return 1;
    }
    memset(c, 0, p->c * sizeof (float));
    return 0;
}

typedef int (*getfn)(img *p, int, int, float *);

static const getfn get[2][4] = {
    { get8u, get16u, NULL, get32f },
    { get8s, get16s, NULL, get32f },
};

//------------------------------------------------------------------------------

// Allocate, initialize, and return an image structure representing a pixel
// buffer with width w, height h, channel count c, bits-per-channel count b,
// and signedness g.

img *img_alloc(int w, int h, int c, int b, int g)
{
    size_t n = (size_t) w * (size_t) h * (size_t) c * (size_t) b / 8;
    img   *p = NULL;

    if ((p = (img *) calloc(1, sizeof (img))))
    {
        if ((p->p = malloc(n)))
        {
            p->sample = img_default;
            p->n = n;
            p->w = w;
            p->h = h;
            p->c = c;
            p->b = b;
            p->g = g;

            p->scaling_factor = 1.0;

            return p;
        }
        else apperr("Failed to allocate image buffer");
    }
    else apperr("Failed to allocate image structure");

    img_close(p);
    return NULL;
}

void img_close(img *p)
{
    if (p)
    {
        if (p->q)
            munmap(p->q, p->n);
        else
            free(p->p);

        if (p->d)
            close(p->d);

        free(p);
    }
}

//------------------------------------------------------------------------------

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
// i, j is smoothly-varying in the range [0, w), [0, h). Return the sample
// coverage.

float img_linear(img *p, float i, float j, float *c)
{
    const int B = (p->b >> 3) - 1;
    const int G =  p->g;

    const float s = i - floorf(i);
    const float t = j - floorf(j);

    const int ia = (int) floorf(i);
    const int ib = (int)  ceilf(i);
    const int ja = (int) floorf(j);
    const int jb = (int)  ceilf(j);

    float aa[4], ab[4];
    float ba[4], bb[4];

    const int kaa = get[G][B](p, ia, ja, aa);
    const int kab = get[G][B](p, ia, jb, ab);
    const int kba = get[G][B](p, ib, ja, ba);
    const int kbb = get[G][B](p, ib, jb, bb);

    switch (p->c)
    {
        case 4: c[3] = lerp2(aa[3], ab[3], ba[3], bb[3], s, t);
        case 3: c[2] = lerp2(aa[2], ab[2], ba[2], bb[2], s, t);
        case 2: c[1] = lerp2(aa[1], ab[1], ba[1], bb[1], s, t);
        case 1: c[0] = lerp2(aa[0], ab[0], ba[0], bb[0], s, t);
    }
    return (kaa || kab || kba || kbb) ? 1.0 : 0.0;
}

//------------------------------------------------------------------------------

static float todeg(float r)
{
    return r * 180.0 / M_PI;
}

#if 0
static float torad(float d)
{
    return d * M_PI / 180.0;
}
#endif

static inline float tolon(float a)
{
    float b = fmodf(a, 2 * M_PI);
    return b < 0 ? b + 2 * M_PI : b;
}

//------------------------------------------------------------------------------

float img_equirectangular(img *p, float lon, float lat, float *c)
{
    float x = p->radius * (lon - p->lonp) * cosf(p->latp);
    float y = p->radius * (lat);

    float l = p->l0 - y / p->scale;
    float s = p->s0 + x / p->scale;

    return img_linear(p, l, s, c);
}

float img_orthographic(img *p, float lon, float lat, float *c)
{
    float x = p->radius * cosf(lat) * sinf(lon - p->lonp);
    float y = p->radius * sinf(lat);

    float l = p->l0 - y / p->scale;
    float s = p->s0 + x / p->scale;

    return img_linear(p, l, s, c);
}

float img_stereographic(img *p, float lon, float lat, float *c)
{
    float x;
    float y;

    if (p->latp > 0)
    {
        x =  2 * p->radius * tanf(M_PI_4 - lat / 2) * sinf(lon - p->lonp);
        y = -2 * p->radius * tanf(M_PI_4 - lat / 2) * cosf(lon - p->lonp);
    }
    else
    {
        x =  2 * p->radius * tanf(M_PI_4 + lat / 2) * sinf(lon - p->lonp);
        y =  2 * p->radius * tanf(M_PI_4 + lat / 2) * cosf(lon - p->lonp);
    }

    float l = p->l0 - y / p->scale;
    float s = p->s0 + x / p->scale;

    return img_linear(p, l, s, c);
}

float img_cylindrical(img *p, float lon, float lat, float *c)
{
    float s = p->s0 + p->res * (todeg(lon) - todeg(p->lonp));
    float l = p->l0 - p->res * (todeg(lat) - todeg(p->latp));

    return img_linear(p, l, s, c);
}

float img_default(img *p, float lon, float lat, float *c)
{
    float l = (p->h - 1) * 0.5f * (M_PI_2 - lat) / M_PI_2;
    float s = (p->w    ) * 0.5f * (M_PI   + lon) / M_PI;

    return img_linear(p, l, s, c);
}

//------------------------------------------------------------------------------

static float blend(float a, float b, float k)
{
    if (a < b)
    {
        if (k < a) return 1.f;
        if (b < k) return 0.f;

        float t = 1.f - (k - a) / (b - a);

        return 3 * t * t - 2 * t * t * t;
    }
    else
    {
        if (k > a) return 1.f;
        if (b > k) return 0.f;

        float t = 1.f - (a - k) / (a - b);

        return 3 * t * t - 2 * t * t * t;
    }
}

static float angle(float a, float b)
{
    float d;

    if (a > b)
    {
        if ((d = a - b) < M_PI)
            return d;
        else
            return 2 * M_PI - d;
    }
    else
    {
        if ((d = b - a) < M_PI)
            return d;
        else
            return 2 * M_PI - d;
    }
}

float img_sample(img *p, const float *v, float *c)
{
    const float lon = tolon(atan2f(v[0], -v[2])), lat = asinf(v[1]);

    const float dlon = angle(lon, p->lonp);
    const float dlat = angle(lat, p->latp);

    float klat  = 1.f;
    float klon  = 1.f;

    if (p->lat0  || p->lat1)  klat = blend(p->lat1,  p->lat0,  lat);
    if (p->lon0  || p->lon1)  klon = blend(p->lon1,  p->lon0,  lon);

    float kdlat = 1.f;
    float kdlon = 1.f;

    if (p->dlat0 || p->dlat1) klat = blend(p->dlat1, p->dlat0, dlat);
    if (p->dlon0 || p->dlon1) klon = blend(p->dlon1, p->dlon0, dlon);

    float k = klat * klon * kdlat * kdlon * p->scaling_factor;

    if (k > 0.f)
    {
        float a = p->sample(p, lon, lat, c);

        switch (p->c)
        {
            case 4: c[3] *= k;
            case 3: c[2] *= k;
            case 2: c[1] *= k;
            case 1: c[0] *= k;
        }
        return a;
    }
    return 0;
}

//------------------------------------------------------------------------------
