#include <stdio.h>

#include "scm.h"
#include "error.h"

#define W 8
#define C 3
#define B 8
#define N ((W + 2) * (W + 2) * C)

int main(int argc, char **argv)
{
	double p[N];
	scm *s;
	int  i;

	setexe(argv[0]);

	for (i = 0; i < N; ++i)
		p[i] = (double) i / N;

	if ((s = scm_ofile("test.tif", W, C, B, 0, "Copyright (C) 2011 Foo")))
	{
		scm_append(s, 0, 0, p);
		scm_close(s);
	}

	memset(p, 0, sizeof (p));

	if ((s = scm_ifile("test.tif")))
	{
		size_t n;
		off_t  o;

		printf("%d %d %d %d '%s'\n",
			scm_get_n(s),
			scm_get_c(s),
			scm_get_b(s),
			scm_get_s(s),
			scm_get_copyright(s));

		if ((o = scm_rewind(s)))
		{
			if ((n = scm_read_page(s, o, p)))
			{
				for (i = 0; i < N; ++i)
					printf("%.2f ", p[i]);
			}
		}

		scm_close(s);
	}

    return 0;
}
