// SCMTIFF Copyright (C) 2012-2015 Robert Kooima
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
#include <ctype.h>
#include <math.h>
#include <sys/mman.h>

#include "img.h"
#include "err.h"
#include "util.h"

#define STRMAX 81

static int equals(const char *a, const char *b)
{
    return (strcasecmp(a, b) == 0) ? 1 : 0;
}

static int get_element(FILE *f, char *k, char *v, char *d)
{
    int n = 1;
    int t;

    while (n && (t = fgetc(f)) != EOF)
        switch (n)
        {
            case 1:
                if      (isspace(t)) { n = 1;           }
                else if (isalpha(t)) { n = 2; *k++ = t; }
                else if (('^' == t)) { n = 2; *k++ = t; }
                else                 { return 0;        }
                break;

            case 2:
                if      (isspace(t)) { n = 3;           }
                else if (isalpha(t)) { n = 2; *k++ = t; }
                else if (('_' == t)) { n = 2; *k++ = t; }
                else                 { return 0;        }
                break;

            case 3:
                if      (isspace(t)) { n = 3;           }
                else if (('=' == t)) { n = 4;           }
                else                 { return 0;        }
                break;

            case 4:
                if      (isspace(t)) { n = 4;           }
                else if (('"' == t)) { n = 5;           }
                else if (isgraph(t)) { n = 6; *v++ = t; }
                else                 { return 0;        }
                break;

            case 5:
                if      (('"' == t)) { n = 7;           }
                else                 { n = 5; *v++ = t; }
                break;

            case 6:
                if      (isgraph(t)) { n = 6; *v++ = t; }
                else if ((015 == t)) { n = 10;          }
                else if ((012 == t)) { n = 0;           }
                else if (isspace(t)) { n = 7;           }
                else                 { return 0;        }
                break;

            case 7:
                if      (('<' == t)) { n = 8;           }
                else if ((015 == t)) { n = 10;          }
                else if ((012 == t)) { n = 0;           }
                else if (isspace(t)) { n = 7;           }
                else                 { return 0;        }
                break;

            case 8:
                if      (('>' == t)) { n = 9;           }
                else if (isgraph(t)) { n = 8; *d++ = t; }
                else                 { return 0;        }
                break;

            case 9:
                if      ((015 == t)) { n = 10;          }
                else if ((012 == t)) { n = 0;           }
                else                 { return 0;        }
                break;

            case 10:
                if      ((012 == t)) { n = 0;           }
                else                 { return 0;        }
                break;
        }

    *k = 0;
    *v = 0;
    *d = 0;

    return n ? 0 : 1;
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

static double get_scale(const char *v, const char *u)
{
    if (equals(u, "M/PIX"))    return get_double(v);
    if (equals(u, "KM/PIX"))   return get_double(v) * 1000.0;
    if (equals(u, "M/PIXEL"))  return get_double(v);
    if (equals(u, "KM/PIXEL")) return get_double(v) * 1000.0;

    return get_double(v);
}

static double get_angle(const char *v, const char *u)
{
    return get_double(v) * M_PI / 180.0;
}

static void parse_file(FILE *f, img *p, const char *lbl, const char *dir)
{
    char k[1024];
    char v[1024];
    char u[1024];
    char img[1024];

    int rs = 0;
    int rc = 0;

    strcpy(img, lbl);

    p->b = 8;
    p->c = 1;
    p->scaling_factor = 1.0f;

    // Read the PDS label and interpret all PDS data elements.

    while (get_element(f, k, v, u))

        if      (equals(k, "RECORD_BYTES"))  rs = get_int(v);
        else if (equals(k, "LABEL_RECORDS")) rc = get_int(v);
        else if (equals(k, "^IMAGE"))
        {
            strcpy(img, dir);
            strcat(img, v);
        }

        // Raster parameters

        else if (equals(k, "LINE_SAMPLES")) p->w = get_int(v);
        else if (equals(k, "LINES"))        p->h = get_int(v);
        else if (equals(k, "BANDS"))        p->c = get_int(v);
        else if (equals(k, "SAMPLE_BITS"))  p->b = get_int(v);
        else if (equals(k, "SAMPLE_TYPE"))
        {
            if      (equals(v, "LSB_INTEGER"))          { p->g = 1; p->o = 0; }
            else if (equals(v, "LSB_UNSIGNED_INTEGER")) { p->g = 0; p->o = 0; }
            else if (equals(v, "MSB_INTEGER"))          { p->g = 1; p->o = 1; }
            else if (equals(v, "MSB_UNSIGNED_INTEGER")) { p->g = 0; p->o = 1; }
            else if (equals(v, "IEEE_REAL"))            { p->g = 0; p->o = 1; }
        }

        // Projection parameters

        else if (equals(k, "MAXIMUM_LATITUDE"))
                         p->maximum_latitude         = get_angle(v, u);
        else if (equals(k, "MINIMUM_LATITUDE"))
                         p->minimum_latitude         = get_angle(v, u);
        else if (equals(k, "CENTER_LATITUDE"))
                         p->center_latitude          = get_angle(v, u);
        else if (equals(k, "EASTERNMOST_LONGITUDE"))
                         p->easternmost_longitude    = get_angle(v, u);
        else if (equals(k, "WESTERNMOST_LONGITUDE"))
                         p->westernmost_longitude    = get_angle(v, u);
        else if (equals(k, "CENTER_LONGITUDE"))
                         p->center_longitude         = get_angle(v, u);
        else if (equals(k, "MAP_SCALE"))
                         p->map_scale                = get_scale(v, u);
        else if (equals(k, "MAP_RESOLUTION"))
                         p->map_resolution           = get_double(v);
        else if (equals(k, "LINE_PROJECTION_OFFSET"))
                         p->line_projection_offset   = get_double(v);
        else if (equals(k, "SAMPLE_PROJECTION_OFFSET"))
                         p->sample_projection_offset = get_double(v);
        else if (equals(k, "A_AXIS_RADIUS"))
                         p->a_axis_radius            = get_double(v) * 1000.0;
        else if (equals(k, "SCALING_FACTOR"))
                         p->scaling_factor           = get_float(v);
        else if (equals(k, "OFFSET"))
                         p->offset                   = get_float(v);

        else if (equals(k, "MAP_PROJECTION_TYPE"))
        {
            if       (equals(v, "EQUIRECTANGULAR"))
                p->project = img_equirectangular;
            else if  (equals(v, "ORTHOGRAPHIC"))
                p->project = img_orthographic;
            else if  (equals(v, "POLAR_STEREOGRAPHIC"))
                p->project = img_polar_stereographic;
            else if  (equals(v, "SIMPLE_CYLINDRICAL"))
                p->project = img_simple_cylindrical;
            else if  (equals(v, "POLAR STEREOGRAPHIC"))
                p->project = img_polar_stereographic;
            else if  (equals(v, "SIMPLE CYLINDRICAL"))
                p->project = img_simple_cylindrical;
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
    char path[256];

    FILE *f = NULL;
    img  *p = NULL;

    dircpy(path, name);
    
    if ((f = fopen(name, "r")))
    {
        if ((p = (img *) calloc(1, sizeof (img))))
        {
            p->project               = img_default;
            p->minimum_latitude      = -0.5 * M_PI;
            p->maximum_latitude      =  0.5 * M_PI;
            p->westernmost_longitude =  0.0 * M_PI;
            p->easternmost_longitude =  2.0 * M_PI;
            p->scaling_factor        =  1.f;
            p->offset                =  0.f;

            p->norm0 = 0.f;
            p->norm1 = 1.f;

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
