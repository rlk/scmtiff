// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <regex.h>
#include <ctype.h>
#include <math.h>
#include <sys/mman.h>

#include "img.h"
#include "err.h"

#define STRMAX 81

//------------------------------------------------------------------------------

// Str is a string giving a PDS element. Pattern is a regular expression with
// two capture groups. If the string matches the pattern, copy the first capture
// to the key string and the second to the val string.

static int get_pair(const char *str, char *key, char *val, const char *pattern)
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

static int get_element(const char *str, char *key, char *val)
{
    return (get_pair(str, key, val, "^ *([A-Z^][A-Z_:]*) *= *\"([^\"]*)\"") ||
            get_pair(str, key, val, "^ *([A-Z^][A-Z_:]*) *= *(.*)$"));
}

// Read a line from the given PDS file. PDS is non-case-sensitive so convert
// it to upper case to simplify regex matching. Trim any trailing whitespace.
// The string END signals the end of PDS label data. Stop there to avoid
// parsing into concatenated raw data.

static int get_line(char *str, size_t len, FILE *fp)
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

static int get_int(const char *val)
{
    return (int) strtol(val, NULL, 0);
}

static float get_real(const char *val)
{
    return (float) strtod(val, NULL);
}

static float get_angle(const char *str)
{
    char val[STRMAX];
    char dim[STRMAX];

    if (get_pair(str, val, dim, "(-?[\\.0-9]+) <([^>]+)>"))
    {
        if (strcmp(dim, "DEG"))
            return get_real(val);
        else
            return get_real(val) * M_PI / 180.0;
    }
    return 0;
}

static void parse_element(img *p, const char *key, const char *val)
{
    const float K = 1000.0f;

    // Raster parameters

    if      (!strcmp(key, "LINE_SAMPLES")) p->w = get_int(val);
    else if (!strcmp(key, "LINES"))        p->h = get_int(val);
    else if (!strcmp(key, "BANDS"))        p->c = get_int(val);
    else if (!strcmp(key, "SAMPLE_BITS"))  p->b = get_int(val);
    else if (!strcmp(key, "SAMPLE_TYPE"))
    {
        if      (!strcmp(val, "LSB_INTEGER")) p->g = 1;
        else if (!strcmp(val, "MSB_INTEGER")) p->g = 1;
    }

    // Projection parameters

    else if (!strcmp(key,     "MAXIMUM_LATITUDE"))   p->latmax = get_angle(val);
    else if (!strcmp(key,     "MINIMUM_LATITUDE"))   p->latmin = get_angle(val);
    else if (!strcmp(key,      "CENTER_LATITUDE"))   p->latp   = get_angle(val);
    else if (!strcmp(key, "EASTERNMOST_LONGITUDE"))  p->lonmax = get_angle(val);
    else if (!strcmp(key, "WESTERNMOST_LONGITUDE"))  p->lonmin = get_angle(val);
    else if (!strcmp(key,      "CENTER_LONGITUDE"))  p->lonp   = get_angle(val);
    else if (!strcmp(key,         "MAP_SCALE"))      p->scale  = get_real (val);
    else if (!strcmp(key,         "MAP_RESOLUTION")) p->res    = get_real (val);
    else if (!strcmp(key,   "LINE_PROJECTION_OFFSET"))   p->l0 = get_real (val);
    else if (!strcmp(key, "SAMPLE_PROJECTION_OFFSET"))   p->s0 = get_real (val);
    else if (!strcmp(key,      "A_AXIS_RADIUS")) p->radius = K * get_real (val);
    else if (!strcmp(key, "SCALING_FACTOR")) p->scaling_factor = get_real (val);

    else if (!strcmp(key, "MAP_PROJECTION_TYPE"))
    {
        if      (!strcmp(val, "EQUIRECTANGULAR"))
            p->sample = img_equirectangular;
        else if (!strcmp(val, "ORTHOGRAPHIC"))
            p->sample = img_orthographic;
        else if (!strcmp(val, "POLAR STEREOGRAPHIC"))
            p->sample = img_stereographic;
        else if (!strcmp(val, "SIMPLE CYLINDRICAL"))
            p->sample = img_cylindrical;
    }
}

static void parse_file(FILE *f, img *p, const char *lbl)
{
    char str[STRMAX];
    char key[STRMAX];
    char val[STRMAX];
    char img[STRMAX];

    int rs = 0;
    int rc = 0;

    strcpy(img, lbl);

    p->b = 8;
    p->c = 1;
    p->scaling_factor = 1.0;

    // Read the PDS label and interpret all PDS data elements.

    while (get_line(str, STRMAX, f))
    {
        if (get_element(str, key, val))
        {
            if      (!strcmp(key, "RECORD_BYTES"))  rs = get_int(val);
            else if (!strcmp(key, "LABEL_RECORDS")) rc = get_int(val);
            else if (!strcmp(key, "FILE_NAME"))          strcpy(img, val);

            else parse_element(p, key, val);
        }
    }

    // Open and map the data file.

    void  *q;
    int    d;
    size_t o = strcmp(img, lbl) ? 0 : rc * rs;
    size_t n = (size_t) p->w * (size_t) p->h
             * (size_t) p->c * (size_t) p->b / 8;

    if ((d = open(img, O_RDONLY)) != -1)
    {
        if ((q = mmap(0, o + n, PROT_READ, MAP_PRIVATE, d, 0)) != MAP_FAILED)
        {
            p->p = (char *) q + o;
            p->q = q;
            p->n = n;
            p->d = d;
        }
        else syserr("Failed to mmap '%s'", img);
    }
    else syserr("Failed to open '%s'", img);
}

//------------------------------------------------------------------------------

img *pds_load(const char *name)
{
    FILE *f = NULL;
    img  *p = NULL;

    if ((f = fopen(name, "r")))
    {
        if ((p = (img *) calloc(1, sizeof (img))))
        {
            parse_file(f, p, name);
            return p;
        }
        else apperr("Failed to allocate image structure");
    }
    else syserr("Failed to open PDS '%s'", name);

    fclose(f);
    return NULL;
}

//------------------------------------------------------------------------------
