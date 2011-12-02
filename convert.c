// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scm.h"
#include "img.h"

//------------------------------------------------------------------------------

int process(scm *s, img *p)
{
	return 0;
}

int extcmp(const char *name, const char *ext)
{
	return 0;
}

int convert(int argc, char **argv)
{
	const char *t = "Copyright (c) 2011 Robert Kooima";
	const char *o = "out.tif";
	int         n = 512;
	int         d = 0;

	img *p = NULL;
	scm *s = NULL;

	int i;

	for (i = 1; i < argc; ++i)
		if      (strcmp(argv[i], "-o") == 0) o =      argv[++i];
		else if (strcmp(argv[i], "-t") == 0) t =      argv[++i];
		else if (strcmp(argv[i], "-n") == 0) n = atoi(argv[++i]);
		else if (strcmp(argv[i], "-d") == 0) d = atoi(argv[++i]);

	if      (extcmp(argv[argc - 1], ".jpg") == 0) p = jpg_load(argv[argc - 1]);
	else if (extcmp(argv[argc - 1], ".png") == 0) p = png_load(argv[argc - 1]);
	else if (extcmp(argv[argc - 1], ".tif") == 0) p = tif_load(argv[argc - 1]);
	else if (extcmp(argv[argc - 1], ".img") == 0) p = img_load(argv[argc - 1]);
	else if (extcmp(argv[argc - 1], ".lbl") == 0) p = lbl_load(argv[argc - 1]);

	if (p)
		s = scm_ofile(o, n, img_get_c(p),
		                    img_get_b(p),
		                    img_get_s(p), t);
    if (s && p)
    	process(s, p);

    scm_close(s);
    img_close(p);

	return 0;
}

//------------------------------------------------------------------------------
