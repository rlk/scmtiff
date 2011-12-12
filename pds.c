// Copyright (c) 2011 Robert Kooima.  All Rights Reverved.

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <regex.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "img.h"
#include "err.h"

#define STRMAX 81

//------------------------------------------------------------------------------

static int getpair(const char *str, char *key, char *val, const char *pattern)
{
    char       error[STRMAX];
    regmatch_t match[3];
    regex_t    regex;

    int err;

    if ((err = regcomp(&regex, pattern, REG_EXTENDED)) == 0)
    {
        if ((err = regexec(&regex, str, 3, match, 0)) == 0)
        {
            memset(key, 0, STRMAX);
            memset(val, 0, STRMAX);

            strncpy(key, str + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
            strncpy(val, str + match[2].rm_so, match[2].rm_eo - match[2].rm_so);

            return 1;
        }
    }

    if (err != REG_NOMATCH && regerror(err, &regex, error, STRMAX))
        apperr("%s : %s", pattern, error);

    return 0;
}

static int getelement(const char *str, char *key, char *val)
{
    return (getpair(str, key, val, "^ *([A-Z^][A-Z_:]*) *= *\"([^\"]*)\"") ||
            getpair(str, key, val, "^ *([A-Z^][A-Z_:]*) *= *(.*)$"));
}

static int getline(char *str, size_t len, FILE *fp)
{
    if (fgets(str, len, fp))
    {
        char *t;

        for (t = str; isprint(*t); ++t) *t = toupper(*t);
        for (       ; isspace(*t); ++t) *t = 0;

        return strcmp(str, "END");
    }
    return 0;
}

static img *parse(FILE *fp)
{
    char str[STRMAX];
    char key[STRMAX];
    char val[STRMAX];

    img *p = NULL;
    int  w = 0;
    int  h = 0;
    int  c = 0;
    int  b = 0;
    int  s = 0;

    while (getline(str, STRMAX, fp))
    {
        if (getelement(str, key, val))
        {
            printf("[%s] [%s]\n", key, val);
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------

img *lbl_load(const char *name)
{
    return img_load(name);;
}

img *img_load(const char *name)
{
    img  *p  = NULL;
    FILE *fp = NULL;

    if ((fp = fopen(name, "r")))
    {
        p = parse(fp);
        fclose(fp);
    }
    else syserr("Failed to open PDS '%s'", name);

    return p;
}

#if 0
{
    img         *p = NULL;
    void        *d = NULL;
    size_t       n = 0;
    int          f = 0;
    struct stat st;

    if ((stat(name, &st)) != -1 && (n = st.st_size))
    {
        if ((f = open(name, O_RDONLY)) != -1)
        {
            if ((d = mmap(0, n, PROT_READ, MAP_PRIVATE, f, 0)) != MAP_FAILED)
            {
                p = parse((const char *) d, n);
            }
            else syserr("failed mmap %s", name);
        }
        else syserr("Failed to open %s", name);
    }
    else syserr("Failed to stat %s", name);

    munmap(d, n);
    close(f);

    return p;
}
#endif

//------------------------------------------------------------------------------
