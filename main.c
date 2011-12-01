#include <stdio.h>
#include <string.h>

#include "scm.h"
#include "error.h"

#define W 8
#define C 3
#define B 8
#define N ((W + 2) * (W + 2) * C)

double p[N];

void report(scm *s, off_t o)
{
	off_t c[4];

	memset(p, 0, sizeof (p));

	off_t  n = scm_read_node(s, o, c);
	size_t l = scm_read_page(s, o, p);

	printf("%llx : [%ld] (%llx %llx %llx %llx) -> %llx\n",
			o, l, c[0], c[1], c[2], c[3], n);
}

void depth_first(scm *s, off_t o)
{
	off_t c[4];
	off_t n;

	if (o)
	{
		report(s, o);

		n = scm_read_node(s, o, c);

		depth_first(s, c[0]);
		depth_first(s, c[1]);
		depth_first(s, c[2]);
		depth_first(s, c[3]);
	}
}

void breadth_first(scm *s, off_t o)
{
	off_t c[4];

	while (o)
	{
		report(s, o);
		o = scm_read_node(s, o, c);
	}
}

int main(int argc, char **argv)
{
	scm *s;
	int  i;

	setexe(argv[0]);

	for (i = 0; i < N; ++i)
		p[i] = (double) i / N;

	if ((s = scm_ofile("test.tif", W, C, B, 0, "Copyright (C) 2011 Foo")))
	{
		off_t o0 = scm_append(s,  0,  0, 0, p);
		off_t o1 = scm_append(s, o0, o0, 0, p);
		off_t o2 = scm_append(s, o1, o0, 1, p);
		off_t o3 = scm_append(s, o2, o0, 2, p);
		off_t o4 = scm_append(s, o3, o0, 3, p);
		off_t o5 = scm_append(s, o4, o1, 0, p);
		off_t o6 = scm_append(s, o5, o1, 1, p);
		off_t o7 = scm_append(s, o6, o1, 2, p);
		off_t o8 = scm_append(s, o7, o1, 3, p);
		scm_close(s);
	}

	if ((s = scm_ifile("test.tif")))
	{
		off_t o;

		if ((o = scm_rewind(s)))
			depth_first(s, o);

		printf("\n");

		if ((o = scm_rewind(s)))
			breadth_first(s, o);

		scm_close(s);
	}

    return 0;
}
