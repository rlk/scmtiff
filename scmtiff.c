// SCMTIFF Copyright (C) 2012 Robert Kooima
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITH-
// OUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include "getopt.h"
#else
#include <getopt.h>
#include <unistd.h>
#endif

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
    const char *m    = NULL;
    const char *o    = NULL;
    const char *t    = NULL;
    int         n    = 512;
    int         d    =   0;
    int         b    =  -1;
    int         g    =  -1;
    int         A    =   0;
    int         h    =   0;
    int         l    =   0;
    int         T    =   0;
    int         x    =   0;
    double      E[4] = { 0.f, 0.f, 0.f , 0.f};
    double      L[3] = { 0.f, 0.f, 0.f };
    double      P[3] = { 0.f, 0.f, 0.f };
    float       N[2] = { 0.f, 0.f };
    float       R[2] = { 0.f, 1.f };

    int c;
    int r = 0;

    gettimeofday(&t0, NULL);

    setexe(exe);

    opterr = 0;

    while ((c = getopt(argc, argv, "Ab:d:E:g:hL:l:m:n:N:o:p:P:Tt:R:x:w:")) != -1)
        switch (c)
        {
            case 'A': A = 1;                    break;
            case 'h': h = 1;                    break;
            case 'T': T = 1;                    break;
            case 'p': p = optarg;               break;
            case 'm': m = optarg;               break;
            case 'o': o = optarg;               break;
            case 't': t = optarg;               break;
            case 'n': sscanf(optarg, "%d", &n); break;
            case 'd': sscanf(optarg, "%d", &d); break;
            case 'b': sscanf(optarg, "%d", &b); break;
            case 'g': sscanf(optarg, "%d", &g); break;
            case 'l': sscanf(optarg, "%d", &l); break;
            case 'x': sscanf(optarg, "%d", &x); break;

            case 'E':
                sscanf(optarg, "%lf,%lf,%lf,%lf", E + 0, E + 1, E + 2, E + 3);
                break;
            case 'L':
                sscanf(optarg, "%lf,%lf,%lf",     L + 0, L + 1, L + 2);
                break;
            case 'P':
                sscanf(optarg, "%lf,%lf,%lf",     P + 0, P + 1, P + 2);
                break;
            case 'N':
                sscanf(optarg, "%f,%f",           N + 0, N + 1);
                break;
            case 'R':
                sscanf(optarg, "%f,%f",           R + 0, R + 1);
                break;

            case '?': apperr("Bad option -%c", optopt);                   break;
        }

    argc -= optind;
    argv += optind;

    if (p == NULL || h)
        apperr("\nUsage: %s [options] input [...]\n"
                "\t\t-p process . . Select process\n"
                "\t\t-o output  . . Output file\n"
                "\t\t-T . . . . . . Emit timing information\n\n"
                "\t%s -p extrema\n\n"
                "\t%s -p convert [options]\n"
                "\t\t-n n . . . . . Page size\n"
                "\t\t-d d . . . . . Tree depth\n"
                "\t\t-b b . . . . . Channel depth override\n"
                "\t\t-g g . . . . . Channel sign override\n"
                "\t\t-E w,e,s,n . . Equirectangular range\n"
                "\t\t-L c,d0,d1 . . Longitude blend range\n"
                "\t\t-P c,d0,d1 . . Latitude blend range\n"
                "\t\t-N n0,n1 . . . Normalization range\n"
                "\t\t-A . . . . . . Coverage alpha\n\n"
                "\t%s -p combine [-m mode]\n"
                "\t\t-m sum . . . . Combine by sum\n"
                "\t\t-m max . . . . Combine by maximum\n"
                "\t\t-m avg . . . . Combine by average\n"
                "\t\t-m blend . . . Combine by alpha blending\n\n"
                "\t%s -p mipmap [-m mode]\n\n"
                "\t\t-m sum . . . . Combine by sum\n"
                "\t\t-m max . . . . Combine by maximum\n"
                "\t\t-m avg . . . . Combine by average\n\n"
                "\t%s -p border\n\n"
                "\t%s -p finish [options]\n"
                "\t\t-t text  . . . Image description text file\n"
                "\t\t-l l . . . . . Bounding volume oversample level\n\n"
                "\t%s -p normal [options]\n"
                "\t\t-R r0,r1 . . . Radius range\n",

                exe, exe, exe, exe, exe, exe, exe, exe);

    else if (strcmp(p, "extrema") == 0)
        r = extrema(argc, argv);

    else if (strcmp(p, "convert") == 0)
        r = convert(argc, argv, o, n, d, b, g, x, A, N, E, L, P);

    else if (strcmp(p, "rectify") == 0)
        r = rectify(argc, argv, o, n,                N, E, L, P);

    else if (strcmp(p, "combine") == 0)
        r = combine(argc, argv, o, m);

    else if (strcmp(p, "mipmap") == 0)
        r = mipmap (argc, argv, o, m, A);

    else if (strcmp(p, "border") == 0)
        r = border (argc, argv, o);

    else if (strcmp(p, "finish") == 0)
        r = finish (argc, argv, t, l);

    else if (strcmp(p, "polish") == 0)
        r = polish (argc, argv);

    else if (strcmp(p, "normal") == 0)
        r = normal (argc, argv, o, R);

    else if (strcmp(p, "sample") == 0)
        r = sample (argc, argv, R, d);

    else apperr("Unknown process '%s'", p);

    gettimeofday(&t1, NULL);

    if (T) printtime((t1.tv_sec  - t0.tv_sec) +
                     (t1.tv_usec - t0.tv_usec) / 1000000.0);

    return r;
}
