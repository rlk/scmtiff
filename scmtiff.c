// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "scm.h"
#include "err.h"

//------------------------------------------------------------------------------

int normal (int, char **, const char *, const float *);
int border (int, char **, const char *);
int mipmap (int, char **, const char *);
int combine(int, char **, const char *, const char *);
int convert(int, char **, const char *, const char *,
            int, int, int, int, const float *, const float *, const float *);

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    const char *p    = NULL;
    const char *m    = "sum";
    const char *o    = NULL;
    const char *t    = NULL;
    int         n    = 512;
    int         d    = 0;
    int         b    = 0;
    int         g    = 0;
    int         h    = 0;
    float       L[3] = { 0.f, 0.f, 0.f };
    float       P[3] = { 0.f, 0.f, 0.f };
    float       N[2] = { 0.f, 0.f };
    float       R[2] = { 0.f, 1.f };

    int c;

    setexe(argv[0]);

    opterr = 0;

    while ((c = getopt(argc, argv, "b:d:g:hL:m:n:N:o:p:P:t:R:")) != -1)
        switch (c)
        {
            case 'p': p = optarg;                                       break;
            case 'm': m = optarg;                                       break;
            case 'o': o = optarg;                                       break;
            case 't': t = optarg;                                       break;
            case 'n': sscanf(optarg, "%d", &n);                         break;
            case 'd': sscanf(optarg, "%d", &d);                         break;
            case 'b': sscanf(optarg, "%d", &b);                         break;
            case 'g': sscanf(optarg, "%d", &g);                         break;
            case 'L': sscanf(optarg, "%f,%f,%f", L + 0, L + 1, L + 2);  break;
            case 'P': sscanf(optarg, "%f,%f,%f", P + 0, P + 1, P + 2);  break;
            case 'N': sscanf(optarg, "%f,%f",    N + 0, N + 1);         break;
            case 'R': sscanf(optarg, "%f,%f",    R + 0, R + 1);         break;
            case 'h': h = 1;                                            break;
            case '?': apperr("Bad option -%c", optopt);             break;
        }

    if (p == NULL || h)
        apperr("\nUsage: %s [-p process] [-o output] [options] input [...]\n\n"
                "\t%s -p convert [options]\n"
                "\t\t-t text  . . . Image description text file\n"
                "\t\t-n n . . . . . Page size\n"
                "\t\t-d d . . . . . Tree depth\n"
                "\t\t-b b . . . . . Channel depth override\n"
                "\t\t-g g . . . . . Channel sign override\n"
                "\t\t-L c,d0,d1 . . Longitude blend range\n"
                "\t\t-P c,d0,d1 . . Latitude blend range\n"
                "\t\t-N n0,n1 . . . Normalization range\n\n"
                "\t%s -p combine [-m mode]\n"
                "\t\t-m max . . . . Combine by maximum\n"
                "\t\t-m sum . . . . Combine by sum\n\n"
                "\t%s -p mipmap [options]\n\n"
                "\t%s -p border [options]\n\n"
                "\t%s -p normal [options]\n"
                "\t\t-R r0,r1 . . . Radius range\n",

                argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);

    else if (strcmp(p, "convert") == 0)
        return convert(argc - optind, argv + optind, o, t, n, d, b, g, L, P, N);

    else if (strcmp(p, "combine") == 0)
        return combine(argc - optind, argv + optind, o, m);

    else if (strcmp(p, "mipmap") == 0)
        return mipmap (argc - optind, argv + optind, o);

    else if (strcmp(p, "border") == 0)
        return border (argc - optind, argv + optind, o);

    else if (strcmp(p, "normal") == 0)
        return normal (argc - optind, argv + optind, o, R);

    else apperr("Unknown process '%s'", p);

    return 0;
}
