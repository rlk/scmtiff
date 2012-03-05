// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"

#define SUM(a, b) ((a) + (b))

//------------------------------------------------------------------------------

// This structure encapsulates an input file. We need to retain more than just
// the SCM structures and their internal state, we should also cache the index
// offset mappings and associated depths.

struct input
{
    scm   *s;
    int    d;
    int    n;
    off_t *m;
};

// Attempt to read and map the SCM TIFF with the given name. If successful,
// append it to the given list. Return the number of elements in the list.

static int enlist(struct input *v, int c, const char *name)
{
    static int N = 0;
    static int C = 0;

    scm   *s;
    int    d;
    off_t *m;

    if ((s = scm_ifile(name)))
    {
        if ((N == 0            && C == 0) ||
            (N == scm_get_n(s) && C == scm_get_c(s)))
        {
            N = scm_get_n(s);
            C = scm_get_c(s);

            if ((d = scm_mapping(s, &m)) >= 0)
            {
                v[c].n = scm_get_page_count(d);
                v[c].s = s;
                v[c].d = d;
                v[c].m = m;

                return ++c;
            }
        }
        else apperr("SCM TIFF '%s' has size %d chan %d."
                            " Expected size %d chan %d.",
                    name, scm_get_n(s), scm_get_c(s), N, C);

        scm_close(s);
    }
    return c;
}

// Sum all SCMs given by the input array. Write the output to SCM s.

static void process(scm *s, struct input *v, int c, int m)
{
    const int N = scm_get_n(s) + 2;
    const int C = scm_get_c(s);

    float *p;
    float *q;

    // Allocate temporary and accumulator buffers.

    if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
    {
        off_t b = 0;
        int   d = 0;

        // Determine the depth of the output.

        for (int i = 0; i < c; ++i)
            if (d < v[i].d)
                d = v[i].d;

        // Process each page of an SCM with the desired depth.

        for (int x = 0; x < scm_get_page_count(d); ++x)
        {
            off_t o = 0;
            scm  *t = 0;
            int   k = 0;

            // Count the number of SCMs contributing to page x.

            for (int i = 0; i < c; ++i)
                if (x < v[i].n && v[i].m[x])
                {
                    o = v[i].m[x];
                    t = v[i].s;
                    k = k + 1;
                }

            // If there is exactly one contributor, repeat its page.

            if (k == 1)
                b = scm_repeat(s, b, t, o);

            // If there is more than one, append their summed pages.

            else if (k > 1)
            {
                memset(p, 0, N * N * C * sizeof (float));

                for (int i = 0; i < c; ++i)
                    if (x < v[i].n && v[i].m[x])

                        if (scm_read_page(v[i].s, v[i].m[x], q))
                        {
                            if (m)
                                for (int j = 0; j < N * N * C; ++j)
                                    p[j] = MAX(p[j], q[j]);
                            else
                                for (int j = 0; j < N * N * C; ++j)
                                    p[j] = SUM(p[j], q[j]);
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
    struct input *v = NULL;
    int           l = 0;
    int           M = 0;

    const char *out = o ? o : "out.tif";

    if (m)
    {
        if      (strcmp(m, "sum") == 0) M = 0;
        else if (strcmp(m, "max") == 0) M = 1;
    }

    if ((v = (struct input *) calloc(argc, sizeof (struct input))))
    {
        for (int i = 0; i < argc; ++i)
            l = enlist(v, l, argv[i]);

        if (l)
        {
            int   n = scm_get_n(v[0].s);
            int   c = scm_get_c(v[0].s);
            int   b = scm_get_b(v[0].s);
            int   g = scm_get_g(v[0].s);
            char *T = scm_get_description(v[0].s);

            scm *s;

            if ((s = scm_ofile(out, n, c, b, g, T)))
            {
                process(s, v, l, M);
                scm_close(s);
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
