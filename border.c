// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
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

static void cpy(float *p, const float *q, int c)
{
    switch (c)
    {
        case 4: p[3] = q[3];
        case 3: p[2] = q[2];
        case 2: p[1] = q[1];
        case 1: p[0] = q[0];
    }
}

static void avg3(float *p, const float *q,
                           const float *r,
                           const float *s, int c)
{
    switch (c)
    {
        case 4: p[3] = (q[3] + r[3] + s[3]) / 3.f;
        case 3: p[2] = (q[2] + r[2] + s[2]) / 3.f;
        case 2: p[1] = (q[1] + r[1] + s[1]) / 3.f;
        case 1: p[0] = (q[0] + r[0] + s[0]) / 3.f;
    }
}

static void copyu(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int j = 0; j < n; ++j)
        cpy(pixel(p, n, c, 0, j),
            pixel(q, n, c, translate_i[x][y](n - 2, j, n),
                           translate_j[x][y](n - 2, j, n)), c);
}

static void copyd(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int j = 0; j < n; ++j)
        cpy(pixel(p, n, c, n - 1, j),
            pixel(q, n, c, translate_i[x][y](1, j, n),
                           translate_j[x][y](1, j, n)), c);
}

static void copyr(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int i = 0; i < n; ++i)
        cpy(pixel(p, n, c, i, 0),
            pixel(q, n, c, translate_i[x][y](i, n - 2, n),
                           translate_j[x][y](i, n - 2, n)), c);
}

static void copyl(float *p, long long x, float *q, long long y, int n, int c)
{
    for (int i = 0; i < n; ++i)
        cpy(pixel(p, n, c, i, n - 1),
            pixel(q, n, c, translate_i[x][y](i, 1, n),
                           translate_j[x][y](i, 1, n)), c);
}

static void process(scm *s, scm *t)
{
    const int o = scm_get_n(s) + 2;
    const int c = scm_get_c(s);

    long long b = 0;
    long long l;
    scm_pair *a;
    float    *p;
    float    *q;

    if ((l = scm_read_catalog(s, &a)))
    {
        scm_sort_catalog(a, l);

        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(t)))
        {
            for (long long i = 0; i < l; ++i)
            {
                if (scm_read_page(s, a[i].o, p))
                {
                    // Determine the page indices of all adjacent pages.

                    long long x = a[i].x;
                    long long xU;
                    long long xD;
                    long long xR;
                    long long xL;

                    scm_get_page_neighbors(x, &xU, &xD, &xR, &xL);

                    // Seek the page catalog locations of these pages.

                    long long iU = scm_seek_catalog(a, 0, l, xU);
                    long long iD = scm_seek_catalog(a, 0, l, xD);
                    long long iR = scm_seek_catalog(a, 0, l, xR);
                    long long iL = scm_seek_catalog(a, 0, l, xL);

                    // Note the file offsets of any located catalog entries.

                    long long oU = (iU < 0) ? 0 : a[iU].o;
                    long long oD = (iD < 0) ? 0 : a[iD].o;
                    long long oR = (iR < 0) ? 0 : a[iR].o;
                    long long oL = (iL < 0) ? 0 : a[iL].o;

                    // Copy the borders of all adjacent pages into this one.

                    if (oU && scm_read_page(s, oU, q))
                        copyu(p, scm_get_page_root(x),
                              q, scm_get_page_root(xU), o, c);
                    if (oD && scm_read_page(s, oD, q))
                        copyd(p, scm_get_page_root(x),
                              q, scm_get_page_root(xD), o, c);
                    if (oR && scm_read_page(s, oR, q))
                        copyr(p, scm_get_page_root(x),
                              q, scm_get_page_root(xR), o, c);
                    if (oL && scm_read_page(s, oL, q))
                        copyl(p, scm_get_page_root(x),
                              q, scm_get_page_root(xL), o, c);

                    // Patch up the corners with an average.

                    const int y = o - 2;
                    const int z = o - 1;

                    avg3(pixel(p, o, c, 0, 0), pixel(p, o, c, 1, 0),
                         pixel(p, o, c, 0, 1), pixel(p, o, c, 1, 1), c);
                    avg3(pixel(p, o, c, z, 0), pixel(p, o, c, y, 0),
                         pixel(p, o, c, z, 1), pixel(p, o, c, y, 1), c);
                    avg3(pixel(p, o, c, 0, z), pixel(p, o, c, 1, z),
                         pixel(p, o, c, 0, y), pixel(p, o, c, 1, y), c);
                    avg3(pixel(p, o, c, z, z), pixel(p, o, c, y, z),
                         pixel(p, o, c, z, y), pixel(p, o, c, y, y), c);

                    // Write the resulting page to the output.

                    b = scm_append(t, b, x, p);
                }
            }
            free(q);
            free(p);
        }
        free(a);
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
            int   n = scm_get_n(s);
            int   c = scm_get_c(s);
            int   b = scm_get_b(s);
            int   g = scm_get_g(s);
            char *T = scm_get_description(s);

            if ((t = scm_ofile(out, n, c, b, g, T)))
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
