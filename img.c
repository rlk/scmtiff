// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
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

static int normu8(const img *p, uint8_t d, float *f)
{
    *f = ((float) d * p->scaling_factor - p->dnorm) * p->knorm;
    return 1;
}

static int norms8(const img *p, int8_t d, float *f)
{
    *f = ((float) d * p->scaling_factor - p->dnorm) * p->knorm;
    return 1;
}

static int normu16(const img *p, uint16_t d, float *f)
{
    *f = ((float) d * p->scaling_factor - p->dnorm) * p->knorm;
    return 1;
}

static int norms16(const img *p, int16_t d, float *f)
{
    if      (d == -32768) return 0;  // Core null
    else if (d == -32767) *f = 0.0;  // Representation saturation low
    else if (d == -32766) *f = 0.0;  // Instrumentation saturation low
    else if (d == -32764) *f = 1.0;  // Representation saturation high
    else if (d == -32765) *f = 1.0;  // Instrumentation saturation high

    else *f = ((float) d * p->scaling_factor - p->dnorm) * p->knorm;

    return 1;
}

static float normf(const img *p, float d, float *f)
{
    unsigned int *w = (unsigned int *) (&d);

    if      (*w == 0xFF7FFFFB) return 0;  // Core null
    else if (*w == 0xFF7FFFFC) *f = 0.0;  // Representation saturation low
    else if (*w == 0xFF7FFFFD) *f = 0.0;  // Instrumentation saturation low
    else if (*w == 0xFF7FFFFE) *f = 1.0;  // Representation saturation high
    else if (*w == 0xFF7FFFFF) *f = 1.0;  // Instrumentation saturation high

    else if (isnormal(d)) *f = (d * p->scaling_factor - p->dnorm) * p->knorm;
    else return 0;

    return 1;
}

//------------------------------------------------------------------------------

// TODO: complete this for all types.

static int getchan(const img *p, int i, int j, int k, float *f)
{
    const int s = p->c * (p->w * i + j);

    if (p->b == 8)
    {
        if (p->g)
            return norms8(p, (( int8_t *) p->p + s)[k], f);
        else
            return normu8(p, ((uint8_t *) p->p + s)[k], f);
    }
    if (p->b == 16)
    {
        if (p->g)
            return norms16(p, (( int16_t *) p->p + s)[k], f);
        else
            return normu16(p, ((uint16_t *) p->p + s)[k], f);
    }
    return normf(p, ((float *) p->p + s)[k], f);
}

static  int getsamp(img *p, int i, int j, float *c)
{
    int d = 0;

    if (0 <= i && i < p->h && 0 <= j && j < p->w)
    {
        switch (p->c)
        {
            case 4: d |= getchan(p, i, j, 3, c + 3);
            case 3: d |= getchan(p, i, j, 2, c + 2);
            case 2: d |= getchan(p, i, j, 1, c + 1);
            case 1: d |= getchan(p, i, j, 0, c + 0);
        }
    }
    return d;
}

// Perform a linearly-filtered sampling of the image p. The filter position
// i, j is smoothly-varying in the range [0, w), [0, h). Return the sample
// coverage.

float img_linear(img *p, float i, float j, float *c)
{
    const int ia = (int) floorf(i);
    const int ib = (int)  ceilf(i);
    const int ja = (int) floorf(j);
    const int jb = (int)  ceilf(j);

    float aa[4], ab[4];
    float ba[4], bb[4];

    int kaa = getsamp(p, ia, ja, aa);
    int kab = getsamp(p, ia, jb, ab);
    int kba = getsamp(p, ib, ja, ba);
    int kbb = getsamp(p, ib, jb, bb);

    if (kaa && kab && kba && kbb)
    {
        const float s = i - floorf(i);
        const float t = j - floorf(j);

        switch (p->c)
        {
            case 4: c[3] = lerp2(aa[3], ab[3], ba[3], bb[3], s, t);
            case 3: c[2] = lerp2(aa[2], ab[2], ba[2], bb[2], s, t);
            case 2: c[1] = lerp2(aa[1], ab[1], ba[1], bb[1], s, t);
            case 1: c[0] = lerp2(aa[0], ab[0], ba[0], bb[0], s, t);
        }
        return 1.0;
    }
    else if (kaa)
    {
        switch (p->c)
        {
            case 4: c[3] = aa[3];
            case 3: c[2] = aa[2];
            case 2: c[1] = aa[1];
            case 1: c[0] = aa[0];
        }
        return 1.0;
    }
    else if (kab)
    {
        switch (p->c)
        {
            case 4: c[3] = ab[3];
            case 3: c[2] = ab[2];
            case 2: c[1] = ab[1];
            case 1: c[0] = ab[0];
        }
        return 1.0;
    }
    else if (kba)
    {
        switch (p->c)
        {
            case 4: c[3] = ba[3];
            case 3: c[2] = ba[2];
            case 2: c[1] = ba[1];
            case 1: c[0] = ba[0];
        }
        return 1.0;
    }
    else if (kbb)
    {
        switch (p->c)
        {
            case 4: c[3] = bb[3];
            case 3: c[2] = bb[2];
            case 2: c[1] = bb[1];
            case 1: c[0] = bb[0];
        }
        return 1.0;
    }
    return 0.0;
//  return (kaa && kab && kba && kbb) ? 1.0 : 0.0;
//  return (kaa || kab || kba || kbb) ? 1.0 : 0.0;
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

    // WAT
    // l -= 1;
    // s -= 1;

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

    // WAT
    // l -= 1;
    // s -= 1;

    return img_linear(p, l, s, c);
}

float img_cylindrical(img *p, float lon, float lat, float *c)
{
    float s = p->s0 + p->res * (todeg(lon) - todeg(p->lonp));
    float l = p->l0 - p->res * (todeg(lat) - todeg(p->latp));

    return img_linear(p, l, s, c);
}

// If panoramas come out reversed, it's because this function hasn't been fixed.

float img_default(img *p, float lon, float lat, float *c)
{
    float l = (p->h - 1) * 0.5f * (M_PI_2 - lat) / M_PI_2;
    float s = (p->w    ) * 0.5f * (M_PI   + lon) / M_PI;

    return img_linear(p, l, s, c);
}

// Panoramas are spheres viewed from the inside while planets are spheres
// viewed from the outside. This difference reverses the handedness of the
// coordinate system. The default projection is "inside" and is applied to
// panorama images, which do not contain projection specifications. All other
// projections are "outside" and are applied to planets, which are usually
// aquired in PDS format, which does contain a projection specification. This is
// the means by which the handedness of the coordinate system is inferred. Be
// advised that this will fail in obscure cases.

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
//  const float lon = tolon(atan2f(v[0], -v[2])), lat = asinf(v[1]);
    const float lon = tolon(atan2f(v[0], v[2])), lat = asinf(v[1]);

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

    float k;
    float a;

    if ((k = klat * klon * kdlat * kdlon))
    {
        if ((a = p->sample(p, lon, lat, c)))
        {
            switch (p->c)
            {
                case 4: c[3] *= k;
                case 3: c[2] *= k;
                case 2: c[1] *= k;
                case 1: c[0] *= k;
            }
            return a;
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
