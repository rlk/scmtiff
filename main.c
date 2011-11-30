#include <stdio.h>

#include "scm.h"
#include "error.h"

int main(int argc, char **argv)
{
	scm *s;

	setexe(argv[0]);

	if ((s = scm_ofile("test.tif", 512, 3, 1, 0, "Copyright (C) 2011 Foo")))
	{
		scm_close(s);
	}

	if ((s = scm_ifile("test.tif")))
	{
		printf("%d %d %d %d '%s'\n",
			scm_get_n(s),
			scm_get_c(s),
			scm_get_b(s),
			scm_get_s(s),
			scm_get_copyright(s));

		scm_close(s);
	}

    return 0;
}
