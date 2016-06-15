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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "util.h"

//------------------------------------------------------------------------------

void normalize(double *v)
{
    double k = 1.0 / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    v[0] *= k;
    v[1] *= k;
    v[2] *= k;
}

//------------------------------------------------------------------------------
// 2-vector and 4-vector normalized midpoint vector calculators

void mid2(double *m, const double *a, const double *b)
{
    m[0] = a[0] + b[0];
    m[1] = a[1] + b[1];
    m[2] = a[2] + b[2];

    normalize(m);
}

void mid4(double *m, const double *a, const double *b,
                    const double *c, const double *d)
{
    m[0] = a[0] + b[0] + c[0] + d[0];
    m[1] = a[1] + b[1] + c[1] + d[1];
    m[2] = a[2] + b[2] + c[2] + d[2];

    normalize(m);
}

//------------------------------------------------------------------------------
// 1D and 2D linear interpolation

float lerp1(float a, float b, float t)
{
    return b * t + a * (1.0f - t);
}

float lerp2(float a, float b, float c, float d, float s, float t)
{
    return lerp1(lerp1(a, b, t), lerp1(c, d, t), s);
}

//------------------------------------------------------------------------------

int extcmp(const char *name, const char *ext)
{
    return strcasecmp(name + strlen(name) - strlen(ext), ext);
}

void dircpy(char *dst, const char *src)
{
    char d = 0;

    for (int i = (int) strlen(src); i >= 0; i--)
    {
        if (src[i] == '/')
            d = 1;
        dst[i] = src[i] * d;
    }
}

//------------------------------------------------------------------------------

static void accumulate(float *p, const float *q, int c)
{
    switch (c)
    {
        case 4: p[3] += q[3];
        case 3: p[2] += q[2];
        case 2: p[1] += q[1];
        case 1: p[0] += q[0];
    }
}

static void divide(float *p, float d, int c)
{
    switch (c)
    {
        case 4: p[3] /= d;
        case 3: p[2] /= d;
        case 2: p[1] /= d;
        case 1: p[0] /= d;
    }
}

// Grow the c-channel, n-by-n image at buffer p by one pixel, taking the average
// of all existing neighbor pixels. Use q as temporary storage.

int grow(float *p, float *q, int c, int n)
{
    const int s = (n + 2) * c;
    const int a = (c - 1);

    float *P;
    float *Q;

    int N = 0;
    int i;
    int j;
    int d;

    memcpy(q, p, (size_t) (n + 2) * (n + 2) * c * sizeof (float));

    #pragma omp parallel for private(j, P, Q, d) reduction(+:N)
    for     (i = 1; i <= n; i++)
        for (j = 1; j <= n; j++)
        {
            P = p + ((n + 2) * i + j) * c;
            Q = q + ((n + 2) * i + j) * c;

            if (Q[a] == 0)
            {
                d = 0;

                if ((Q - s - c)[a]) { accumulate(P, Q - s - c, a); d++; }
                if ((Q - s    )[a]) { accumulate(P, Q - s    , a); d++; }
                if ((Q - s + c)[a]) { accumulate(P, Q - s + c, a); d++; }

                if ((Q     - c)[a]) { accumulate(P, Q     - c, a); d++; }
                if ((Q     + c)[a]) { accumulate(P, Q     + c, a); d++; }

                if ((Q + s - c)[a]) { accumulate(P, Q + s - c, a); d++; }
                if ((Q + s    )[a]) { accumulate(P, Q + s    , a); d++; }
                if ((Q + s + c)[a]) { accumulate(P, Q + s + c, a); d++; }

                if (d)
                {
                    divide(P, (float) d, a);
                    N++;
                }
            }
        }
    return N;
}

//------------------------------------------------------------------------------

int report_done = 0;
int report_todo = 0;

void hms(char *str, int s)
{
    int h = 0;
    int m = 0;

    if (s > 60.0)
    {
        if (s > 3600.0)
        {
            h  = (int) s / 3600;
            s -= (int) h * 3600;
        }

        m  = (int) s / 60;
        s -= (int) m * 60;
    }

    if      (h) sprintf(str, "%dh %dm %ds", h, m, s);
    else if (m) sprintf(str,     "%dm %ds",    m, s);
    else        sprintf(str,         "%ds",       s);
}

void report_init(int n)
{
    report_done = 0;
    report_todo = n;
}

void report_step(void)
{
    report_done++;

    if (report_todo)
    {
        static char   str[256];
        static int    dirty = 0;
        static double start = 0;

        char passed[256];
        char remain[256];

        if (dirty)
        {
            for (int i = 0; i < strlen(str); i++)
                printf("\b");
        }
        else
        {
            start = now();
            dirty = 1;
        }

        double d = now() - start;

        hms(passed, d);
        hms(remain, report_done ? d * report_todo / report_done - d : 0);

        sprintf(str, "Processed %d of %d (%.2f%%) %s passed, "
                                     "estimated %s remaining. ",
                    report_done,
                    report_todo, 100.0 * report_done / report_todo,
                    passed, remain);

        if (report_done < report_todo)
            printf("%s", str);
        else
            printf("%s\n", str);

        fflush(stdout);
    }
}

//------------------------------------------------------------------------------
