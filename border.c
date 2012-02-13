// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "util.h"

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

static double *pixel(double *p, int n, int c, int i, int j)
{
    return p + c * (i * n + j);
}

static void cpy(double *p, const double *q, int c)
{
    switch (c)
    {
        case 4: p[3] = q[3];
        case 3: p[2] = q[2];
        case 2: p[1] = q[1];
        case 1: p[0] = q[0];
    }
}

static void avg3(double *p, const double *q,
                            const double *r,
                            const double *s, int c)
{
    switch (c)
    {
        case 4: p[3] = (q[3] + r[3] + s[3]) / 3.0;
        case 3: p[2] = (q[2] + r[2] + s[2]) / 3.0;
        case 2: p[1] = (q[1] + r[1] + s[1]) / 3.0;
        case 1: p[0] = (q[0] + r[0] + s[0]) / 3.0;
    }
}

static void copyu(double *p, int x, double *q, int y, int n, int c)
{
    for (int j = 0; j < n; ++j)
        cpy(pixel(p, n, c, 0, j),
            pixel(q, n, c, translate_i[x][y](n - 2, j, n),
                           translate_j[x][y](n - 2, j, n)), c);
}

static void copyd(double *p, int x, double *q, int y, int n, int c)
{
    for (int j = 0; j < n; ++j)
        cpy(pixel(p, n, c, n - 1, j),
            pixel(q, n, c, translate_i[x][y](1, j, n),
                           translate_j[x][y](1, j, n)), c);
}

static void copyr(double *p, int x, double *q, int y, int n, int c)
{
    for (int i = 0; i < n; ++i)
        cpy(pixel(p, n, c, i, 0),
            pixel(q, n, c, translate_i[x][y](i, n - 2, n),
                           translate_j[x][y](i, n - 2, n)), c);
}

static void copyl(double *p, int x, double *q, int y, int n, int c)
{
    for (int i = 0; i < n; ++i)
        cpy(pixel(p, n, c, i, n - 1),
            pixel(q, n, c, translate_i[x][y](i, 1, n),
                           translate_j[x][y](i, 1, n)), c);
}

static int process(scm *s, scm *t)
{
    const int n = scm_get_n(s) + 2;
    const int c = scm_get_c(s);

    off_t   b = 0;
    off_t  *m;
    double *p;
    double *q;
    int     d;

    if ((d = scm_mapping(s, &m)) >= 0)
    {
        if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(t)))
        {
            for (int x = 0; x < scm_get_page_count(d); ++x)
            {
                if (m[x] && scm_read_page(s, m[x], p))
                {
                    // Copy the borders of all adjacent pages into this one.

                    int U, D, R, L;

                    scm_get_page_neighbors(x, &U, &D, &R, &L);

                    if (m[U] && scm_read_page(s, m[U], q))
                        copyu(p, scm_get_page_root(x),
                              q, scm_get_page_root(U), n, c);
                    if (m[D] && scm_read_page(s, m[D], q))
                        copyd(p, scm_get_page_root(x),
                              q, scm_get_page_root(D), n, c);
                    if (m[R] && scm_read_page(s, m[R], q))
                        copyr(p, scm_get_page_root(x),
                              q, scm_get_page_root(R), n, c);
                    if (m[L] && scm_read_page(s, m[L], q))
                        copyl(p, scm_get_page_root(x),
                              q, scm_get_page_root(L), n, c);

                    // Patch up the corners with an average.

                    const int y = n - 2;
                    const int z = n - 1;

                    avg3(pixel(p, n, c, 0, 0), pixel(p, n, c, 1, 0),
                         pixel(p, n, c, 0, 1), pixel(p, n, c, 1, 1), c);
                    avg3(pixel(p, n, c, z, 0), pixel(p, n, c, y, 0),
                         pixel(p, n, c, z, 1), pixel(p, n, c, y, 1), c);
                    avg3(pixel(p, n, c, 0, z), pixel(p, n, c, 1, z),
                         pixel(p, n, c, 0, y), pixel(p, n, c, 1, y), c);
                    avg3(pixel(p, n, c, z, z), pixel(p, n, c, y, z),
                         pixel(p, n, c, z, y), pixel(p, n, c, y, y), c);

                    // Write the resulting page to the output.

                    b = scm_append(t, b, 0, 0, x, p);
                }
            }
            free(q);
            free(p);
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}

//------------------------------------------------------------------------------

int border(int argc, char **argv)
{
    const char *out = "out.tif";
    const char *in  = "in.tif";

    scm *s = NULL;
    scm *t = NULL;

    int r = EXIT_FAILURE;

    for (int i = 1; i < argc; ++i)
        if      (strcmp(argv[i],   "-o") == 0) out = argv[++i];
        else if (extcmp(argv[i], ".tif") == 0) in  = argv[  i];

    if ((s = scm_ifile(in)))
    {
        char *str = scm_get_description(s);

        int n = scm_get_n(s);
        int c = scm_get_c(s);
        int b = scm_get_b(s);
        int g = scm_get_g(s);

        if ((t = scm_ofile(out, n, c, b, g, str)))
        {
            r = process(s, t);
            scm_relink(t);
            scm_minmax(t);
            scm_close(t);
        }
        scm_close(s);
    }
    return r;
}

//------------------------------------------------------------------------------
