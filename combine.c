// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"
#include "process.h"

#define sub(a, b) ((a) + (b))

//------------------------------------------------------------------------------

// This structure encapsulates an input file. We need to retain more than just
// the SCM structures and their internal state, we should also cache the index
// offset mappings and associated depths.

struct file
{
    scm      *s;
    scm_pair *a;
    long long l;
};

// Attempt to read and map the SCM TIFF with the given name. If successful,
// append it to the given list. Return the number of elements in the list.

static int addfile(struct file *F, int C, const char *name)
{
    static int n = 0;
    static int c = 0;

    scm      *s;
    scm_pair *a;
    long long l;

    if ((s = scm_ifile(name)))
    {
        if ((n == 0            && c == 0) ||
            (n == scm_get_n(s) && c == scm_get_c(s)))
        {
            n = scm_get_n(s);
            c = scm_get_c(s);

            if ((l = scm_scan_catalog(s, &a)) >= 0)
            {
                scm_sort_catalog(a, l);
                F[C].s = s;
                F[C].a = a;
                F[C].l = l;
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

static void process(scm *s, struct file *F, int C, int M)
{
    const size_t S = (size_t) (scm_get_n(s) + 2)
                   * (size_t) (scm_get_n(s) + 2)
                   * (size_t) (scm_get_c(s));

    float *p;
    float *q;

    // Allocate temporary and accumulator buffers.

    if ((p = scm_alloc_buffer(s)) && (q = scm_alloc_buffer(s)))
    {
        long long b = 0;
        long long m = 0;
        long long i[C];
        long long o[C];

        // Determine the highest page index in the input.

        for (int f = 0; f < C; ++f)
        {
            if (m < F[f].a[F[f].l - 1].x)
                m = F[f].a[F[f].l - 1].x;

            i[f] = 0;
            o[f] = 0;
        }

        // Process each page of an SCM with the desired depth.

        for (int x = 0; x <= m; ++x)
        {
            int g = 0;
            int k = 0;

            // Count the SCMs contributing to page x.

            for (int f = 0; f < C; ++f)

                if ((i[f] = scm_seek_catalog(F[f].a, i[f] + 1, F[f].l, x)) >= 0)
                {
                    o[f] = F[f].a[i[f]].o;
                    g = f;
                    k++;
                }
                else
                    o[f] = 0;

            // If there is exactly one contributor, repeat its page.

            if (k == 1)
                b = scm_repeat(s, b, F[g].s, o[g]);

            // If there is more than one, append their summed pages.

            else if (k > 1)
            {
                memset(p, 0, S * sizeof (float));

                for (int f = 0; f < C; ++f)
                    if (o[f] && scm_read_page(F[f].s, o[f], q))
                    {
                        if (M)
                            for (size_t j = 0; j < S; ++j)
                                p[j] = max(p[j], q[j]);
                        else
                            for (size_t j = 0; j < S; ++j)
                                p[j] = sub(p[j], q[j]);
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
    struct file *F = NULL;
    int          C = 0;
    int          M = 0;

    const char *out = o ? o : "out.tif";

    if (m)
    {
        if      (strcmp(m, "sum") == 0) M = 0;
        else if (strcmp(m, "max") == 0) M = 1;
    }

    if ((F = (struct file *) calloc((size_t) argc, sizeof (struct file))))
    {
        for (int i = 0; i < argc; ++i)
            C = addfile(F, C, argv[i]);

        if (C)
        {
            int   n = scm_get_n(F[0].s);
            int   c = scm_get_c(F[0].s);
            int   b = scm_get_b(F[0].s);
            int   g = scm_get_g(F[0].s);
            char *T = scm_get_description(F[0].s);

            scm *s;

            if ((s = scm_ofile(out, n, c, b, g, T)))
            {
                process(s, F, C, M);
                scm_close(s);
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
