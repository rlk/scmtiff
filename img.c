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
            p->project = img_default;
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

// Close the image file and release any mapped or allocated buffers.

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
    *f = ((float) d * p->scaling_factor - p->norm0)
                            / (p->norm1 - p->norm0);
    return 1;
}

static int norms8(const img *p, int8_t d, float *f)
{
    *f = ((float) d * p->scaling_factor - p->norm0)
                            / (p->norm1 - p->norm0);
    return 1;
}

static int normu16(const img *p, uint16_t d, float *f)
{
    *f = ((float) d * p->scaling_factor - p->norm0)
                            / (p->norm1 - p->norm0);
    return 1;
}

static int norms16(const img *p, int16_t d, float *f)
{
    if      (d == -32768) return 0;  // Core null
    else if (d == -32767) *f = 0.0;  // Representation saturation low
    else if (d == -32766) *f = 0.0;  // Instrumentation saturation low
    else if (d == -32764) *f = 1.0;  // Representation saturation high
    else if (d == -32765) *f = 1.0;  // Instrumentation saturation high

    else *f = ((float) d * p->scaling_factor - p->norm0)
                                 / (p->norm1 - p->norm0);

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

    else if (isnormal(d)) *f = (d * p->scaling_factor - p->norm0)
                                          / (p->norm1 - p->norm0);
    else return 0;

    return 1;
}

//------------------------------------------------------------------------------

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

static int getsamp(img *p, int i, int j, float *c)
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
// is smoothly-varying in the range [0, w), [0, h).

static int img_linear(img *p, const float *t, float *c)
{
    const int ia = (int) floorf(t[0]);
    const int ib = (int)  ceilf(t[0]);
    const int ja = (int) floorf(t[1]);
    const int jb = (int)  ceilf(t[1]);

    float aa[4], ab[4];
    float ba[4], bb[4];

    int kaa = getsamp(p, ia, ja, aa);
    int kab = getsamp(p, ia, jb, ab);
    int kba = getsamp(p, ib, ja, ba);
    int kbb = getsamp(p, ib, jb, bb);

    if (kaa && kab && kba && kbb)
    {
        const float u = t[0] - floorf(t[0]);
        const float v = t[1] - floorf(t[1]);

        switch (p->c)
        {
            case 4: c[3] = lerp2(aa[3], ab[3], ba[3], bb[3], u, v);
            case 3: c[2] = lerp2(aa[2], ab[2], ba[2], bb[2], u, v);
            case 2: c[1] = lerp2(aa[1], ab[1], ba[1], bb[1], u, v);
            case 1: c[0] = lerp2(aa[0], ab[0], ba[0], bb[0], u, v);
        }
        return 1;
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
        return 1;
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
        return 1;
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
        return 1;
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
        return 1;
    }
    return 0;
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

void img_equirectangular(img *p, float lon, float lat, float *t)
{
    float x = p->radius * (lon - p->lonp) * cosf(p->latp);
    float y = p->radius * (lat);

    t[0] = p->l0 - y / p->scale;
    t[1] = p->s0 + x / p->scale;
}

void img_orthographic(img *p, float lon, float lat, float *t)
{
    float x = p->radius * cosf(lat) * sinf(lon - p->lonp);
    float y = p->radius * sinf(lat);

    t[0] = p->l0 - y / p->scale;
    t[1] = p->s0 + x / p->scale;
}

void img_stereographic(img *p, float lon, float lat, float *t)
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

    t[0] = p->l0 - y / p->scale;
    t[1] = p->s0 + x / p->scale;
}

void img_cylindrical(img *p, float lon, float lat, float *t)
{
    t[0] = p->l0 - p->res * (todeg(lat) - todeg(p->latp));
    t[1] = p->s0 + p->res * (todeg(lon) - todeg(p->lonp));
}

void img_default(img *p, float lon, float lat, float *t)
{
    t[0] = (p->h - 1) * (M_PI_4 - 0.5f * lat) / M_PI_2;
    t[1] = (p->w    ) * (M_PI   - 0.5f * lon) / M_PI;
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

int img_sample(img *p, const float *v, float *c)
{
    const float lon = tolon(atan2f(v[0], v[2])), lat = asinf(v[1]);

    float klat = 1.f;
    float klon = 1.f;

    if (p->latc || p->lat0 || p->lat1)
        klat = blend(p->lat0, p->lat1, angle(lat, p->latc));
    if (p->lonc || p->lon0 || p->lon1)
        klon = blend(p->lon0, p->lon1, angle(lon, p->lonc));

    float k;

    if ((k = klat * klon))
    {
        float t[2];

        p->project(p, lon, lat, t);

        if (img_linear(p, t, c))
        {
            switch (p->c)
            {
                case 4: c[3] *= k;
                case 3: c[2] *= k;
                case 2: c[1] *= k;
                case 1: c[0] *= k;
            }
            return 1;
        }
    }
    return 0;
}

int img_locate(img *p, const float *v)
{
    const float lon = tolon(atan2f(v[0], v[2])), lat = asinf(v[1]);

    float t[2];

    p->project(p, lon, lat, t);

    if (0 <= t[0] && t[0] < p->h && 0 <= t[1] && t[1] < p->w)
        return 1;
    else
        return 0;
}

//------------------------------------------------------------------------------
