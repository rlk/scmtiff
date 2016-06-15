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
#include <stdbool.h>

#include "scm.h"
#include "scmdat.h"
#include "scmdef.h"

//------------------------------------------------------------------------------

// Determine whether page x is a leaf of scm s. That is: if x has data but
// has no children with data.

static bool isleaf(scm *s, long long i)
{
    if (scm_get_offset(s, i))
    {
        long long x = scm_get_index(s, i);
        long long j;

        for (int c = 0; c < 4; c++)
        {
            // If child c exists and has data, then x is not a leaf.

            if ((j = scm_search(s, scm_page_child(x, c))) != -1)
                if (scm_get_offset(s, j) >= 0)
                    return false;
        }
        return true;
    }
    return false;
}

int query(int argc, char **argv)
{
    long long total = 0;

    // Iterate over all input file arguments.

    for (int argi = 0; argi < argc; argi++)
    {
        long long size   = 0;
        long long length = 0;
        long long leaves = 0;
        scm *s;

        if ((s = scm_ifile(argv[argi])))
        {
            if (scm_read_catalog(s))
            {
                length = scm_get_length(s);

                for (long long i = 0; i < length; i++)
                {
                    if (isleaf(s, i))
                    {
                        leaves += 1;
                        size += (long long) s->n
                              * (long long) s->n
                              * (long long) s->c
                              * (long long) s->b / 8;
                    }
                }
            }
            scm_close(s);
            printf("%s pixels: %d channels: %d bits: %d pages: %lld leaves: %lld bytes: %lld\n", argv[argi], s->n, s->c, s->b, length, leaves, size);
        }

        total += size;
    }
    printf("total bytes: %lld\n", total);

    return 0;
}

//------------------------------------------------------------------------------

