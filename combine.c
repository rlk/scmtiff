// SCMTIFF Copyright (C) 2012 Robert Kooima
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

static inline float sum(float a, float b)
{
    return a + b;
}

static inline float avg(float a, float b)
{
    if (a == 0.f) return b;
    if (b == 0.f) return a;
    return (a + b) * 0.5f;
}

static inline void blend(float *dst, const float *src, int c)
{
    const float a =       src[c - 1];
    const float b = 1.f - src[c - 1];

    switch (c)
    {
        case 4: dst[2] = dst[2] * b + src[2] * a;
        case 3: dst[1] = dst[1] * b + src[1] * a;
        case 2: dst[0] = dst[0] * b + src[0] * a;
    }
    dst[c - 1] = max(dst[c - 1], src[c - 1]);
}

//------------------------------------------------------------------------------

// Attempt to read and map the SCM TIFF with the given name. If successful,
// append it to the given list. Return the number of elements in the list.

static int addfile(scm **V, int C, const char *name)
{
    static int n = 0;
    static int c = 0;

    scm *s;

    if ((s = scm_ifile(name)))
    {
        if ((n == 0            && c == 0) ||
            (n == scm_get_n(s) && c == scm_get_c(s)))
        {
            n = scm_get_n(s);
            c = scm_get_c(s);

            if ((scm_scan_catalog(s)))
            {
                V[C] = s;
                return ++C;
            }
        }
        else apperr("SCM TIFF '%s' has size %d chan %d."
                            " Expected size %d chan %d.",
                    name, scm_get_n(s), scm_get_c(s), n, c);

        scm_close(s);
    }
    return C;
}

// Sum all SCMs given by the input array. Write the output to SCM s.

static void process(scm *s, scm **V, int C, int O)
{
    const size_t S = (size_t) (scm_get_n(s))
                   * (size_t) (scm_get_n(s))
                   * (size_t) (scm_get_c(s));

    float *p;
    float *q;

    // Allocate temporary and accumulator buffers.

    if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
    {
        long long b = 0;
        long long m = 0;
        long long i = 0;
        long long o[C];

        // Determine the highest page index in the input.

        for (int f = 0; f < C; ++f)
        {
            if (m < scm_get_index(V[f], scm_get_length(V[f]) - 1))
                m = scm_get_index(V[f], scm_get_length(V[f]) - 1);

            o[f] = 0;
        }

        // Process each page of an SCM with the desired depth.

        for (int x = 0; x <= m; ++x)
        {
            int c = scm_get_c(s);
            int g = 0;
            int k = 0;

            // Count the SCMs contributing to page x.

            for (int f = 0; f < C; ++f)

                if ((i = scm_search(V[f], x)) < 0)
                    o[f] = 0;
                else
                {
                    o[f] = scm_get_offset(V[f], i);
                    g = f;
                    k++;
                }

            // If there is exactly one contributor, repeat its page.

            if (k == 1)
                b = scm_repeat(s, b, V[g], o[g]);

            // If there is more than one, append their summed pages.

            else if (k > 1)
            {
                memset(p, 0, S * sizeof (float));

                for (int f = 0; f < C; ++f)
                    if (o[f] && scm_read_page(V[f], o[f], q))

                        switch (O)
                        {
                            case 0:
                                for (size_t j = 0; j < S; ++j)
                                    p[j] = sum(p[j], q[j]);
                                break;
                            case 1:
                                for (size_t j = 0; j < S; ++j)
                                    p[j] = max(p[j], q[j]);
                                break;
                            case 2:
                                for (size_t j = 0; j < S; ++j)
                                    p[j] = avg(p[j], q[j]);
                                break;
                            case 3:
                                for (size_t j = 0; j < S; j += c)
                                    blend(p + j, q + j, c);
                                break;
                        }

                b = scm_append(s, b, x, p);
            }
        }

        free(q);
        free(p);
    }
}

//------------------------------------------------------------------------------

int combine(int argc, char **argv, const char *o, const char *m)
{
    scm **V = NULL;
    int   C = 0;
    int   O = 0;

    const char *out = o ? o : "out.tif";

    if (m)
    {
        if      (strcmp(m, "sum")   == 0) O = 0;
        else if (strcmp(m, "max")   == 0) O = 1;
        else if (strcmp(m, "avg")   == 0) O = 2;
        else if (strcmp(m, "blend") == 0) O = 3;
    }

    if ((V = (scm **) calloc((size_t) argc, sizeof (scm *))))
    {
        for (int i = 0; i < argc; ++i)
            C = addfile(V, C, argv[i]);

        if (C)
        {
            int n = scm_get_n(V[0]);
            int c = scm_get_c(V[0]);
            int b = scm_get_b(V[0]);
            int g = scm_get_g(V[0]);

            scm *s;

            if ((s = scm_ofile(out, n, c, b, g)))
            {
                process(s, V, C, O);
                scm_close(s);
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
