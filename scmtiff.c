// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdio.h>
#include <string.h>

#include "scm.h"
#include "err.h"

//------------------------------------------------------------------------------

int convert(int argc, char **argv, char *);
int combine(int argc, char **argv);
int mipmap (int argc, char **argv);
int border (int argc, char **argv);
int normal (int argc, char **argv);

//------------------------------------------------------------------------------

#ifdef CONFMPI
#include <mpi.h>

int main(int argc, char **argv)
{
    if (MPI_Init(&argc, &argv) == MPI_SUCCESS)
    {
        int rank = 0;
        int size = 0;

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        setexe(argv[0]);

        if (argc > 3)
        {
            if (strcmp(argv[1], "-convert") == 0)
            {
                int f;
                int i;

                for (f = 2; f < argc; f++)
                    if (argv[f][0] == '-')
                        f++;
                    else
                        break;

                for (i = f + rank; i < argc; i += size)
                    convert(f - 2, argv + 2, argv[i]);
            }
        }

        MPI_Finalize();
    }
}

#else

int main(int argc, char **argv)
{
    setexe(argv[0]);

    if (argc > 3)
    {
        if      (strcmp(argv[1], "-convert") == 0)
            return convert(argc - 3, argv + 2, argv[argc - 1]);

        else if (strcmp(argv[1], "-combine") == 0)
            return combine(argc - 2, argv + 2);

        else if (strcmp(argv[1], "-mipmap")  == 0)
            return mipmap (argc - 2, argv + 2);

        else if (strcmp(argv[1], "-border")  == 0)
            return border (argc - 2, argv + 2);

        else if (strcmp(argv[1], "-normal")  == 0)
            return normal (argc - 2, argv + 2);
    }
    else
        apperr("\nUsage:"
               "\t%s -convert [-o outfile] [-n samples] [-d depth] infile\n"
               "\t%s -combine [-o outfile]                         infile...\n"
               "\t%s -mipmap  [-o outfile]                         infile\n"
               "\t%s -border  [-o outfile]                         infile\n"
               "\t%s -normal  [-o outfile] [-r0 rad] [-r1 rad]     infile",
            argv[0], argv[0], argv[0], argv[0], argv[0]);

    return 0;
}

#endif