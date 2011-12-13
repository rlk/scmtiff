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

// Str is a string giving a PDS element. Pattern is a regular expression with
// two capture groups. If the string matches the pattern, Copy the first capture
// to the key string and the second to the val string.

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

// Parse a PDS element by attempting to match it against regular expressions.
// The first recognizes quoted strings and returns only the quoted contents in
// the value. The second mops up everything else.

static int getelement(const char *str, char *key, char *val)
{
    return (getpair(str, key, val, "^ *([A-Z^][A-Z_:]*) *= *\"([^\"]*)\"") ||
            getpair(str, key, val, "^ *([A-Z^][A-Z_:]*) *= *(.*)$"));
}

// Read a line from the given PDS file. PDS is non-case-sensitive so convert
// it to upper case to simplify regex matching. Trim any trailing whitespace.
// The string END signals the end of PDS label data. Stop there to avoid
// parsing into concatenated raw data.

static int getline(char *str, size_t len, FILE *fp)
{
    if (fgets(str, len, fp))
    {
        char *t = str;

        for (; isprint(*t); ++t) *t = toupper(*t);
        for (; isspace(*t); ++t) *t = 0;

        return strcmp(str, "END");
    }
    return 0;
}

// Open the named PDS label and parse all necessary data elements. We're
// ignoring a heck of a lot of stuff here, but loading the minimum necessary
// to fit the capabilities of the image sampling system. This also covers all
// the data sets we know we'll need to read.

static img *parse(const char *lbl)
{
	img  *p = NULL;
	FILE *f = NULL;

    if ((f = fopen(lbl, "r")))
    {
	    char str[STRMAX];
	    char key[STRMAX];
	    char val[STRMAX];
	    char img[STRMAX];

	    int  w = 0;
	    int  h = 0;
	    int  c = 1;
	    int  b = 8;
	    int  s = 0;
	    int rs = 0;
	    int rc = 0;

	    strcpy(img, lbl);

	    while (getline(str, STRMAX, f))
	    {
	        if (getelement(str, key, val))
	        {
	        	if      (!strcmp(key, "LINE_SAMPLES"))   w = strtol(val, 0, 0);
	        	else if (!strcmp(key, "LINES"))          h = strtol(val, 0, 0);
	        	else if (!strcmp(key, "BANDS"))          c = strtol(val, 0, 0);
	        	else if (!strcmp(key, "SAMPLE_BITS"))    b = strtol(val, 0, 0);
	        	else if (!strcmp(key, "RECORD_BYTES"))  rs = strtol(val, 0, 0);
	        	else if (!strcmp(key, "LABEL_RECORDS")) rc = strtol(val, 0, 0);
	        	else if (!strcmp(key, "FILE_NAME"))	         strcpy(img, val);

	        	else if (!strcmp(key, "SAMPLE_TYPE"))
	        	{
	        		if      (!strcmp(val, "LSB_INTEGER")) s = 1;
	        		else if (!strcmp(val, "MSB_INTEGER")) s = 1;
	        	}
	        	else if (!strcmp(key, "MAP_PROJECTION_TYPE"))
	        	{
	        	}
	        }
	    }

	    if (w && h && c && b)
	    {
		    if (strcmp(img, lbl))
			    p = img_mmap(w, h, c, b, s, img, 0);
			else
			    p = img_mmap(w, h, c, b, s, img, rc * rs);
		}
        fclose(f);
    }
    else syserr("Failed to open PDS '%s'", name);

    return p;
}

//------------------------------------------------------------------------------

img *lbl_load(const char *name)
{
    return parse(name);
}

img *img_load(const char *name)
{
	return parse(name);
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
