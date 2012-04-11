// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "util.h"
#include "process.h"

//------------------------------------------------------------------------------

int catalog(int argc, char **argv)
{
    for (int i = 0; i < argc; i++)
    {
        scm *s = NULL;

        if ((s = scm_ifile(argv[i])))
        {
            scm_make_catalog(s);
            scm_make_extrema(s);
            scm_close(s);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
