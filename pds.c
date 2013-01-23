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

static int get_match(const char *s, char *v, const char *pattern)
{
    char       error[STRMAX];
    regmatch_t match[2];
    regex_t    regex;

    int err;

    if ((err = regcomp(&regex, pattern, REG_EXTENDED)) == 0)
    {
        if ((err = regexec(&regex, s, 2, match, 0)) == 0)
        {
            memset (v, 0, STRMAX);
            strncpy(v, s + match[1].rm_so, match[1].rm_eo - match[1].rm_so);

            return 1;
        }
    }

    if (err != REG_NOMATCH && regerror(err, &regex, error, STRMAX))
        apperr("%s : %s", pattern, error);

    return 0;
}

// Str is a string giving a PDS element. Pattern is a regular expression with
// two capture groups. If the string matches the pattern, copy the first capture
// to the key string and the second to the val string.

static int get_pair(const char *s, char *k, char *v, const char *pattern)
{
    char       error[STRMAX];
    regmatch_t match[3];
    regex_t    regex;

    int err;

    if ((err = regcomp(&regex, pattern, REG_EXTENDED)) == 0)
    {
        if ((err = regexec(&regex, s, 3, match, 0)) == 0)
        {
            memset(k, 0, STRMAX);
            memset(v, 0, STRMAX);

            strncpy(k, s + match[1].rm_so, match[1].rm_eo - match[1].rm_so);
            strncpy(v, s + match[2].rm_so, match[2].rm_eo - match[2].rm_so);

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

static int get_element(const char *s, char *k, char *v)
{
    return (get_pair(s, k, v, "^ *([A-Z^][A-Z_:]*) *= *\"([^\"]*)\"") ||
            get_pair(s, k, v, "^ *([A-Z^][A-Z_:]*) *= *(.*)$"));
}

// Read a line from the given PDS file. PDS is non-case-sensitive so convert
// it to upper case to simplify regex matching. Trim any trailing whitespace.
// The string END signals the end of PDS label data. Stop there to avoid
// parsing into concatenated raw data.

static int get_line(char *s, int n, FILE *fp)
{
    if (fgets(s, n, fp))
    {
        char *t = s;

        for (; isprint(*t); ++t) *t = (char) toupper(*t);
        for (; isspace(*t); ++t) *t = 0;

        return strcmp(s, "END");
    }
    return 0;
}

static int get_int(const char *v)
{
    return (int) strtol(v, NULL, 0);
}

static float get_float(const char *v)
{
    return (float) strtod(v, NULL);
}

static double get_double(const char *v)
{
    return strtod(v, NULL);
}

static double get_scale(const char *s)
{
    char v[STRMAX];

    if (get_match(s, v, "(-?[\\.0-9]+) <M/PIX>"))
        return get_double(v);
    if (get_match(s, v, "(-?[\\.0-9]+) <KM/PIX>"))
        return get_double(v) * 1000.0;;

    return get_double(s);
}

static double get_angle(const char *s)
{
    char v[STRMAX];

    if (get_match(s, v, "(-?[\\.0-9]+) <DEG>"))
        return get_double(v) * M_PI / 180.0;
    else
        return get_double(s);
}

static void parse_element(img *p, const char *k, const char *v)
{
    const double K = 1000.0;

    // Raster parameters

    if      (!strcmp(k, "LINE_SAMPLES")) p->w = get_int(v);
    else if (!strcmp(k, "LINES"))        p->h = get_int(v);
    else if (!strcmp(k, "BANDS"))        p->c = get_int(v);
    else if (!strcmp(k, "SAMPLE_BITS"))  p->b = get_int(v);
    else if (!strcmp(k, "SAMPLE_TYPE"))
    {
        if      (!strcmp(v, "LSB_INTEGER")) p->g = 1;
        else if (!strcmp(v, "MSB_INTEGER")) p->g = 1;
    }

    // Projection parameters

    else if (!strcmp(k,     "MAXIMUM_LATITUDE"))   p->latmax = get_angle (v);
    else if (!strcmp(k,     "MINIMUM_LATITUDE"))   p->latmin = get_angle (v);
    else if (!strcmp(k,      "CENTER_LATITUDE"))   p->latp   = get_angle (v);
    else if (!strcmp(k, "EASTERNMOST_LONGITUDE"))  p->lonmax = get_angle (v);
    else if (!strcmp(k, "WESTERNMOST_LONGITUDE"))  p->lonmin = get_angle (v);
    else if (!strcmp(k,      "CENTER_LONGITUDE"))  p->lonp   = get_angle (v);
    else if (!strcmp(k,         "MAP_SCALE"))      p->scale  = get_scale (v);
    else if (!strcmp(k,         "MAP_RESOLUTION")) p->res    = get_double(v);
    else if (!strcmp(k,   "LINE_PROJECTION_OFFSET"))   p->l0 = get_double(v);
    else if (!strcmp(k, "SAMPLE_PROJECTION_OFFSET"))   p->s0 = get_double(v);
    else if (!strcmp(k,      "A_AXIS_RADIUS")) p->radius = K * get_double(v);
    else if (!strcmp(k, "SCALING_FACTOR")) p->scaling_factor = get_float (v);

    else if (!strcmp(k, "MAP_PROJECTION_TYPE"))
    {
        if      (!strcmp(v, "EQUIRECTANGULAR"))
            p->project = img_equirectangular;
        else if (!strcmp(v, "ORTHOGRAPHIC"))
            p->project = img_orthographic;
        else if (!strcmp(v, "POLAR STEREOGRAPHIC"))
            p->project = img_stereographic;
        else if (!strcmp(v, "SIMPLE CYLINDRICAL"))
            p->project = img_cylindrical;
    }
}

static void parse_file(FILE *f, img *p, const char *lbl, const char *dir)
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
    p->scaling_factor = 1.0f;

    // Read the PDS label and interpret all PDS data elements.

    while (get_line(str, STRMAX, f))
    {
        if (get_element(str, key, val))
        {
            if      (strcmp(key, "RECORD_BYTES")  == 0)  rs = get_int(val);
            else if (strcmp(key, "LABEL_RECORDS") == 0) rc = get_int(val);
#if 1
            else if (strcmp(key, "FILE_NAME")     == 0)
#else
            else if (strcmp(key, "^IMAGE")        == 0)
#endif
            {
                strcpy(img, dir);
                strcat(img, val);
            }
            else parse_element(p, key, val);
        }
    }

    // Open and map the data file.

    void  *q;
    int    d;
    size_t o = strcmp(img, lbl) ? 0 : (size_t) (rc * rs);
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
    char file[256];
    char path[256];

    FILE *f = NULL;
    img  *p = NULL;

    // Separate the file name from its path.

    if (get_pair(name, path, file, "(.*/)([^/].*)") == 0)
        strcpy(path, "");

    if ((f = fopen(name, "r")))
    {
        if ((p = (img *) calloc(1, sizeof (img))))
        {
            p->norm0          = 0.f;
            p->norm1          = 1.f;
            p->scaling_factor = 1.f;

            parse_file(f, p, name, path);
            return p;
        }
        else apperr("Failed to allocate image structure");
    }
    else syserr("Failed to open PDS '%s'", name);

    fclose(f);
    return NULL;
}

//------------------------------------------------------------------------------
