// SCMTIFF Copyright (C) 2012-2016 Robert Kooima
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
#include "util.h"
#include "scmdef.h"

#include "process.h"

//------------------------------------------------------------------------------

static const float *pixel(const float *p, int n, int c, int i, int j)
{
    return p + c * (i * n + j);
}

static bool necessary(const float *p, int n, int c)
{
    const float *a = pixel(p, n, c, 1, 1);

    for     (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
        {
            const float *b = pixel(p, n, c, i, j);

            switch (c)
            {
                case 4: if (a[3] != b[3]) return true;
                case 3: if (a[2] != b[2]) return true;
                case 2: if (a[1] != b[1]) return true;
                case 1: if (a[0] != b[0]) return true;
            }
        }

    return false;
}

static void forget(scm *s, long long i)
{
    long long x = scm_get_index(s, i);
    long long j;

    scm_forget(s, i);

    if ((j = scm_search(s, scm_page_child(x, 0))) >= 0) forget(s, j);
    if ((j = scm_search(s, scm_page_child(x, 1))) >= 0) forget(s, j);
    if ((j = scm_search(s, scm_page_child(x, 2))) >= 0) forget(s, j);
    if ((j = scm_search(s, scm_page_child(x, 3))) >= 0) forget(s, j);
}

static void process(scm *s, scm *t)
{
    const int n = scm_get_n(s);
    const int c = scm_get_c(s);

    long long b = 0;
    float    *p;

    if (scm_scan_catalog(s))
    {
        report_init((int) scm_get_length(s));

        if ((p = scm_alloc_buffer(s)))
        {
            for (long long i = 0; i < scm_get_length(s); ++i)
            {
                const long long o = scm_get_offset(s, i);
                const long long x = scm_get_index (s, i);

                if (o && scm_read_page(s, o, p))
                {
                    if (scm_page_level(x) == 0 || necessary(p, n + 2, c))
                    {
                        b = scm_append(t, b, x, p);
                    }
                    else
                    {
                        forget(s, i);
                    }
                }
                // report_step();
            }
            free(p);
        }
    }
}

//------------------------------------------------------------------------------

int prune(int argc, char **argv, const char *o)
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
