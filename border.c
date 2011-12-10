// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "util.h"

//------------------------------------------------------------------------------

// Implement a set of functions to write the top, bottom, left, and right.
// Implement a set of functions to read the top, bottom, left, and right, in
// either forward or reverse order. Use table or conditional logic to determine
// source index and read function for a given destination index and write function.

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
        if ((t = scm_ofile(out, scm_get_n(s), scm_get_c(s),
                                scm_get_b(s), scm_get_s(s),
                                scm_get_copyright(s))))
        {
//          r = process(s, t);
            scm_close(t);
        }
        scm_close(s);
    }
    return r;
}

//------------------------------------------------------------------------------
