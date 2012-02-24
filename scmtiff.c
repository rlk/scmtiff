// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <string.h>

#include "scm.h"
#include "err.h"

//------------------------------------------------------------------------------

int convert(int argc, char **argv);
int combine(int argc, char **argv);
int mipmap (int argc, char **argv);
int border (int argc, char **argv);
int normal (int argc, char **argv);

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
	int i;

	setexe(argv[0]);

	for (i = 0; i < argc; ++i)
		printf("%s ", argv[i]);

	printf("\n");

	for (i = 1; i < argc; ++i)
		if      (strcmp(argv[i], "-convert") == 0) return convert(argc, argv);
		else if (strcmp(argv[i], "-combine") == 0) return combine(argc, argv);
		else if (strcmp(argv[i], "-mipmap")  == 0) return mipmap (argc, argv);
		else if (strcmp(argv[i], "-border")  == 0) return border (argc, argv);
		else if (strcmp(argv[i], "-normal")  == 0) return normal (argc, argv);

	apperr("\nUsage:"
		   "\t%s -convert [-o outfile] [-n samples] [-d depth] infile\n"
		   "\t%s -combine [-o outfile]                         infile...\n"
		   "\t%s -mipmap  [-o outfile]                         infile\n"
		   "\t%s -border  [-o outfile]                         infile\n"
		   "\t%s -normal  [-o outfile] [-r0 rad] [-r1 rad]     infile",
		argv[0], argv[0], argv[0], argv[0], argv[0]);

    return 0;
}
