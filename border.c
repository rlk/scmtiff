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
#include "scmdef.h"

#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

typedef int (*tofun)(int, int, int);

static int topi(int i, int j, int n) { return         i; }
static int toni(int i, int j, int n) { return n - 1 - i; }
static int topj(int i, int j, int n) { return         j; }
static int tonj(int i, int j, int n) { return n - 1 - j; }

static const tofun translate_i[6][6] = {
    { topi, NULL, tonj, topj, topi, topi },
    { NULL, topi, topj, tonj, topi, topi },
    { topj, tonj, topi, NULL, topi, toni },
    { tonj, topj, NULL, topi, topi, toni },
    { topi, topi, topi, topi, topi, NULL },
    { topi, topi, toni, toni, NULL, topi },
};

static const  tofun translate_j[6][6] = {
    { topj, NULL, topi, toni, topj, topj },
    { NULL, topj, toni, topi, topj, topj },
    { toni, topi, topj, NULL, topj, tonj },
    { topi, toni, NULL, topj, topj, tonj },
    { topj, topj, topj, topj, topj, NULL },
    { topj, topj, tonj, tonj, NULL, topj },
};

static float *pixel(float *p, int n, int c, int i, int j)
{
    return p + c * (i * n + j);
}

static void sum(float *p, const float *q, int c)
{
    switch (c)
    {
        case 4: p[3] += q[3];
        case 3: p[2] += q[2];
        case 2: p[1] += q[1];
        case 1: p[0] += q[0];
    }
}

static void avg(float *p, int d, int c)
{
    switch (c)
    {
        case 4: p[3] /= d;
        case 3: p[2] /= d;
        case 2: p[1] /= d;
        case 1: p[0] /= d;
    }
}

static void sumn(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int j = 0; j < n; ++j)
        sum(pixel(p, n, c, 0, j),
            pixel(q, n, c, translate_i[x][y](n - 1, j, n),
                           translate_j[x][y](n - 1, j, n)), c);
}

static void sums(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int j = 0; j < n; ++j)
        sum(pixel(p, n, c, n - 1, j),
            pixel(q, n, c, translate_i[x][y](0, j, n),
                           translate_j[x][y](0, j, n)), c);
}

static void sumw(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int i = 0; i < n; ++i)
        sum(pixel(p, n, c, i, 0),
            pixel(q, n, c, translate_i[x][y](i, n - 1, n),
                           translate_j[x][y](i, n - 1, n)), c);
}

static void sume(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int i = 0; i < n; ++i)
        sum(pixel(p, n, c, i, n - 1),
            pixel(q, n, c, translate_i[x][y](i, 0, n),
                           translate_j[x][y](i, 0, n)), c);
}

static void sumnw(float *p, long long x, float *q, long long y, int n, int c)
{
    if (translate_i[x][y])
        sum(pixel(p, n, c, 0, 0),
            pixel(q, n, c, translate_i[x][y](n - 1, n - 1, n),
                           translate_j[x][y](n - 1, n - 1, n)), c);
}

static void sumne(float *p, long long x, float *q, long long y, int n, int c)
{
    if (translate_i[x][y])
        sum(pixel(p, n, c, 0, n - 1),
            pixel(q, n, c, translate_i[x][y](n - 1, 0, n),
                           translate_j[x][y](n - 1, 0, n)), c);
}

static void sumsw(float *p, long long x, float *q, long long y, int n, int c)
{
    if (translate_i[x][y])
        sum(pixel(p, n, c, n - 1, 0),
            pixel(q, n, c, translate_i[x][y](0, n - 1, n),
                           translate_j[x][y](0, n - 1, n)), c);
}

static void sumse(float *p, long long x, float *q, long long y, int n, int c)
{
    if (translate_i[x][y])
        sum(pixel(p, n, c, n - 1, n - 1),
            pixel(q, n, c, translate_i[x][y](0, 0, n),
                           translate_j[x][y](0, 0, n)), c);
}

static void process(scm *s, scm *t)
{
    const int n = scm_get_n(s);
    const int c = scm_get_c(s);
    const int m = scm_get_n(s) - 1;

    long long b = 0;
    float    *p;
    float    *q;

    if (scm_scan_catalog(s))
    {
        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(t)))
        {
            for (long long i = 0; i < scm_get_length(s); ++i)
            {
                if (scm_read_page(s, scm_get_offset(s, i), p))
                {
                    // Determine the page indices of all neighboring pages.

                    long long x   = scm_get_index(s, i);

                    long long xn  = scm_page_north(x);
                    long long xs  = scm_page_south(x);
                    long long xw  = scm_page_west (x);
                    long long xe  = scm_page_east (x);

                    // Determine their roots.

                    long long f   = scm_page_root(x);
                    long long fn  = scm_page_root(xn);
                    long long fs  = scm_page_root(xs);
                    long long fw  = scm_page_root(xw);
                    long long fe  = scm_page_root(xe);

                    // Use roots to guide the determination of diagonals.

                    long long xnw = (fn == f) ? scm_page_west (xn)
                                              : scm_page_north(xw);
                    long long xne = (fn == f) ? scm_page_east (xn)
                                              : scm_page_north(xe);
                    long long xsw = (fs == f) ? scm_page_west (xs)
                                              : scm_page_south(xw);
                    long long xse = (fs == f) ? scm_page_east (xs)
                                              : scm_page_south(xe);

                    // Seek the page catalog locations of all neighbors.

                    long long in  = scm_search(s, xn);
                    long long is  = scm_search(s, xs);
                    long long iw  = scm_search(s, xw);
                    long long ie  = scm_search(s, xe);
                    long long inw = scm_search(s, xnw);
                    long long ine = scm_search(s, xne);
                    long long isw = scm_search(s, xsw);
                    long long ise = scm_search(s, xse);

                    // Get the file offset of all pages.

                    long long on  = (in  < 0) ? 0 : scm_get_offset(s, in);
                    long long os  = (is  < 0) ? 0 : scm_get_offset(s, is);
                    long long ow  = (iw  < 0) ? 0 : scm_get_offset(s, iw);
                    long long oe  = (ie  < 0) ? 0 : scm_get_offset(s, ie);
                    long long onw = (inw < 0) ? 0 : scm_get_offset(s, inw);
                    long long one = (ine < 0) ? 0 : scm_get_offset(s, ine);
                    long long osw = (isw < 0) ? 0 : scm_get_offset(s, isw);
                    long long ose = (ise < 0) ? 0 : scm_get_offset(s, ise);

                    int cn  = 0;
                    int cs  = 0;
                    int cw  = 0;
                    int ce  = 0;
                    int cnw = 0;
                    int cne = 0;
                    int csw = 0;
                    int cse = 0;

                    // Sum the borders of all adjacent pages into this one.

                    if (on && scm_read_page(s, on, q))
                    {
                        sumn(p, f, q, fn, n, c);
                        cn = 1;
                    }
                    if (os && scm_read_page(s, os, q))
                    {
                        sums(p, f, q, fs, n, c);
                        cs = 1;
                    }
                    if (ow && scm_read_page(s, ow, q))
                    {
                        sumw(p, f, q, fw, n, c);
                        cw = 1;
                    }
                    if (oe && scm_read_page(s, oe, q))
                    {
                        sume(p, f, q, fe, n, c);
                        ce = 1;
                    }

                    // Sum the corners of all diagonal pages into this one.

                    long long fnw = scm_page_root(xnw);
                    long long fne = scm_page_root(xne);
                    long long fsw = scm_page_root(xsw);
                    long long fse = scm_page_root(xse);

                    if ((f == fn || f == fw) && onw && scm_read_page(s, onw, q))
                    {
                        sumnw(p, f, q, fnw, n, c);
                        cnw = 1;
                    }
                    if ((f == fn || f == fe) && one && scm_read_page(s, one, q))
                    {
                        sumne(p, f, q, fne, n, c);
                        cne = 1;
                    }
                    if ((f == fs || f == fw) && osw && scm_read_page(s, osw, q))
                    {
                        sumsw(p, f, q, fsw, n, c);
                        csw = 1;
                    }
                    if ((f == fs || f == fe) && ose && scm_read_page(s, ose, q))
                    {
                        sumse(p, f, q, fse, n, c);
                        cse = 1;
                    }

                    // Calculate the averages.

                    for (int j = 1; j < m; j++)
                    {
                        avg(pixel(p, n, c, 0, j), 1 + cn, c);
                        avg(pixel(p, n, c, m, j), 1 + cs, c);
                        avg(pixel(p, n, c, j, 0), 1 + cw, c);
                        avg(pixel(p, n, c, j, m), 1 + ce, c);
                    }

                    avg(pixel(p, n, c, 0, 0), 1 + cn + cw + cnw, c);
                    avg(pixel(p, n, c, 0, m), 1 + cn + ce + cne, c);
                    avg(pixel(p, n, c, m, 0), 1 + cs + cw + csw, c);
                    avg(pixel(p, n, c, m, m), 1 + cs + ce + cse, c);

                    // Write the resulting page to the output.

                    b = scm_append(t, b, x, p);
                }
            }
            free(q);
            free(p);
        }
    }
}

//------------------------------------------------------------------------------

int border(int argc, char **argv, const char *o)
{
    if (argc > 0)
    {
        const char *out = o ? o : "out.tif";

        scm *s = NULL;
        scm *t = NULL;

        if ((s = scm_ifile(argv[0])))
        {
            int n = scm_get_n(s);
            int c = scm_get_c(s);
            int b = scm_get_b(s);
            int g = scm_get_g(s);

            if ((t = scm_ofile(out, n, c, b, g)))
            {
                process(s, t);
                scm_close(t);
            }
            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
