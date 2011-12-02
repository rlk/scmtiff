// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdio.h>
#include <stdlib.h>

#include "img.h"

//------------------------------------------------------------------------------

img *jpg_load(const char *name)
{
	img *p = NULL;

	if ((p = (img *) calloc(1, sizeof (img))))
	{
		p->c = 3;
		p->b = 1;
		p->s = 0;

		p->sample = img_sample_test;
	}

	return p;
}

//------------------------------------------------------------------------------
