// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "scm.h"
#include "err.h"
#include "process.h"

//------------------------------------------------------------------------------

#include <sys/time.h>
#include <sys/resource.h>

static void printhms(double dt)
{
    if (dt > 60.0)
    {
        if (dt > 3600.0)
        {
            int h = (int) dt / 3600;
            dt -= h * 3600;
            printf("%dh", h);
        }

        int m = (int) dt / 60;
        dt -= m * 60;
        printf("%dm", m);
    }

    printf("%2.3fs", dt);
}

static void printtime(double dt)
{
    struct rusage ru;

    if (getrusage(RUSAGE_SELF, &ru) == 0)
    {
        printf("\treal: ");
        printhms(dt);
        printf("  user: ");
        printhms(ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1000000.0);
        printf(" sys: ");
        printhms(ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1000000.0);
        printf("\n");
    }
}

//------------------------------------------------------------------------------

int main(int argc, char **argv)
{
    const char *exe = argv[0];

    struct timeval t0;
    struct timeval t1;

    const char *p    = NULL;
    const char *m    = "sum";
    const char *o    = NULL;
    const char *t    = NULL;
    int         n    = 512;
    int         d    =   0;
    int         b    =  -1;
    int         g    =  -1;
    int         h    =   0;
    int         T    =   0;
    int         x    =  -1;
    double      L[3] = { 0.f, 0.f, 0.f };
    double      P[3] = { 0.f, 0.f, 0.f };
    float       N[2] = { 0.f, 0.f };
    float       R[2] = { 0.f, 1.f };

    int c;
    int r = 0;

    gettimeofday(&t0, NULL);

    setexe(exe);

    opterr = 0;

    while ((c = getopt(argc, argv, "b:d:g:hL:m:n:N:o:p:P:Tt:R:x:")) != -1)
        switch (c)
        {
            case 'p': p = optarg;                                         break;
            case 'm': m = optarg;                                         break;
            case 'o': o = optarg;                                         break;
            case 't': t = optarg;                                         break;
            case 'n': sscanf(optarg, "%d", &n);                           break;
            case 'd': sscanf(optarg, "%d", &d);                           break;
            case 'b': sscanf(optarg, "%d", &b);                           break;
            case 'g': sscanf(optarg, "%d", &g);                           break;
            case 'x': sscanf(optarg, "%d", &x);                           break;
            case 'L': sscanf(optarg, "%lf,%lf,%lf", L + 0, L + 1, L + 2); break;
            case 'P': sscanf(optarg, "%lf,%lf,%lf", P + 0, P + 1, P + 2); break;
            case 'N': sscanf(optarg, "%f,%f",       N + 0, N + 1);        break;
            case 'R': sscanf(optarg, "%f,%f",       R + 0, R + 1);        break;
            case 'h': h = 1;                                              break;
            case 'T': T = 1;                                              break;
            case '?': apperr("Bad option -%c", optopt);                   break;
        }

    argc -= optind;
    argv += optind;

    if (p == NULL || h)
        apperr("\nUsage: %s [options] input [...]\n"
                "\t\t-p process . . Select process\n"
                "\t\t-o output  . . Output file\n"
                "\t\t-T . . . . . . Emit timing information\n\n"
                "\t%s -p convert [options]\n"
                "\t\t-n n . . . . . Page size\n"
                "\t\t-d d . . . . . Tree depth\n"
                "\t\t-b b . . . . . Channel depth override\n"
                "\t\t-g g . . . . . Channel sign override\n"
                "\t\t-L c,d0,d1 . . Longitude blend range\n"
                "\t\t-P c,d0,d1 . . Latitude blend range\n"
                "\t\t-N n0,n1 . . . Normalization range\n\n"
                "\t%s -p combine [-m mode]\n"
                "\t\t-m sum . . . . Combine by sum\n\n"
                "\t\t-m max . . . . Combine by maximum\n"
                "\t\t-m avg . . . . Combine by average\n\n"
                "\t%s -p mipmap\n\n"
                "\t%s -p border\n\n"
                "\t%s -p catalog [options]\n"
                "\t\t-t text  . . . Image description text file\n\n"
                "\t%s -p normal [options]\n"
                "\t\t-R r0,r1 . . . Radius range\n",

                exe, exe, exe, exe, exe, exe, exe);

    else if (strcmp(p, "convert") == 0)
        r = convert(argc, argv, o, n, d, b, g, x, L, P, N);

    else if (strcmp(p, "combine") == 0)
        r = combine(argc, argv, o, m);

    else if (strcmp(p, "mipmap") == 0)
        r = mipmap (argc, argv, o);

    else if (strcmp(p, "border") == 0)
        r = border (argc, argv, o);

    else if (strcmp(p, "catalog") == 0)
        r = catalog (argc, argv);

    else if (strcmp(p, "normal") == 0)
        r = normal (argc, argv, o, R);

    else apperr("Unknown process '%s'", p);

    gettimeofday(&t1, NULL);

    if (T) printtime((t1.tv_sec  - t0.tv_sec) +
                     (t1.tv_usec - t0.tv_usec) / 1000000.0);

    return r;
}
