// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "err.h"
#include "util.h"

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

static int addin(struct input *v, int c, const char *name)
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

//------------------------------------------------------------------------------

// Sum all SCMs given by the input array. Write the output to SCM s.

static void process(scm *s, struct input *v, int c)
{
    const int N = scm_get_n(s) + 2;
    const int C = scm_get_c(s);

    double *p;
    double *q;

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
            memset(p, 0, N * N * C * sizeof (double));

            // Sum all files contributing to this page.

            for (int i = 0; i < c; ++i)
            {
                if (x < v[i].n && v[i].m[x])
                {
                    if (scm_read_page(v[i].s, v[i].m[x], q))
                    {
                        for (int j = 0; j < N * N * C; ++j)
                            p[j] += q[j];
                    }
                }
            }

            // Write the sum to the output.

            b = scm_append(s, b, 0, 0, x, p);
        }

        free(q);
        free(p);
    }
}

//------------------------------------------------------------------------------

int combine(int argc, char **argv)
{
    int           c = 0;
    struct input *v = NULL;

    if ((v = (struct input *) calloc(argc - 1, sizeof (struct input))))
    {
        const char *out = "out.tif";

        for (int i = 1; i < argc; ++i)
            if      (strcmp(argv[i],   "-o") == 0) out = argv[++i];
            else if (extcmp(argv[i], ".tif") == 0) c   = addin(v, c, argv[i]);

        if (c)
        {
            scm *s;

            if ((s = scm_ofile(out, scm_get_n(v[0].s), scm_get_c(v[0].s),
                                    scm_get_b(v[0].s), scm_get_s(v[0].s),
                                    scm_get_copyright(v[0].s))))
            {
                process(s, v, c);
                scm_relink(s);
                scm_close(s);
            }
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
