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

#include <math.h>

#include "scmdef.h"

// Calculate the vector toward (x, y) on root face f. --------------------------

void scm_vector(long long f, double y, double x, double *v)
{
    const double s = x * M_PI / 2.0 - M_PI / 4.0;
    const double t = y * M_PI / 2.0 - M_PI / 4.0;

    double u[3];

    u[0] =  sin(s) * cos(t);
    u[1] = -cos(s) * sin(t);
    u[2] =  cos(s) * cos(t);

    double k = 1.0 / sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);

    u[0] *= k;
    u[1] *= k;
    u[2] *= k;

    switch (f)
    {
        case 0: v[0] =  u[2]; v[1] =  u[1]; v[2] = -u[0]; break;
        case 1: v[0] = -u[2]; v[1] =  u[1]; v[2] =  u[0]; break;
        case 2: v[0] =  u[0]; v[1] =  u[2]; v[2] = -u[1]; break;
        case 3: v[0] =  u[0]; v[1] = -u[2]; v[2] =  u[1]; break;
        case 4: v[0] =  u[0]; v[1] =  u[1]; v[2] =  u[2]; break;
        case 5: v[0] = -u[0]; v[1] =  u[1]; v[2] = -u[2]; break;
    }
}

// Determine the page to the north of page i. ----------------------------------

long long scm_page_north(long long i)
{
    long long l = scm_page_level(i);
    long long f = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - c;

    if      (r >  0) {        r = r - 1;    }
    else if (f == 0) { f = 2; r = t; c = m; }
    else if (f == 1) { f = 2; r = c; c = 0; }
    else if (f == 2) { f = 5; r = 0; c = t; }
    else if (f == 3) { f = 4; r = m;        }
    else if (f == 4) { f = 2; r = m;        }
    else             { f = 2; r = 0; c = t; }

    return scm_page_index(f, l, r, c);
}

// Determine the page to the south of page i. ----------------------------------

long long scm_page_south(long long i)
{
    long long l = scm_page_level(i);
    long long f = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - c;

    if      (r <  m) {        r = r + 1;    }
    else if (f == 0) { f = 3; r = c; c = m; }
    else if (f == 1) { f = 3; r = t; c = 0; }
    else if (f == 2) { f = 4; r = 0;        }
    else if (f == 3) { f = 5; r = m; c = t; }
    else if (f == 4) { f = 3; r = 0;        }
    else             { f = 3; r = m; c = t; }

    return scm_page_index(f, l, r, c);
}

// Determine the page to the west of page i. -----------------------------------

long long scm_page_west(long long i)
{
    long long l = scm_page_level(i);
    long long f = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - r;

    if      (c >  0) {        c = c - 1;    }
    else if (f == 0) { f = 4; c = m;        }
    else if (f == 1) { f = 5; c = m;        }
    else if (f == 2) { f = 1; c = r; r = 0; }
    else if (f == 3) { f = 1; c = t; r = m; }
    else if (f == 4) { f = 1; c = m;        }
    else             { f = 0; c = m;        }

    return scm_page_index(f, l, r, c);
}

// Determine the page to the east of page i. -----------------------------------

long long scm_page_east(long long i)
{
    long long l = scm_page_level(i);
    long long f = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l, m = n - 1, t = m - r;

    if      (c <  m) {        c = c + 1;    }
    else if (f == 0) { f = 5; c = 0;        }
    else if (f == 1) { f = 4; c = 0;        }
    else if (f == 2) { f = 0; c = t; r = 0; }
    else if (f == 3) { f = 0; c = r; r = m; }
    else if (f == 4) { f = 0; c = 0;        }
    else             { f = 1; c = 0;        }

    return scm_page_index(f, l, r, c);
}

// Calculate the four corner vectors of page i. --------------------------------

void scm_page_corners(long long i, double *v)
{
    long long l = scm_page_level(i);
    long long f = scm_page_root(i);
    long long r = scm_page_row(i);
    long long c = scm_page_col(i);

    long long n = 1LL << l;

    scm_vector(f, (double) (r + 0) / n, (double) (c + 0) / n, v + 0);
    scm_vector(f, (double) (r + 0) / n, (double) (c + 1) / n, v + 3);
    scm_vector(f, (double) (r + 1) / n, (double) (c + 0) / n, v + 6);
    scm_vector(f, (double) (r + 1) / n, (double) (c + 1) / n, v + 9);
}

//------------------------------------------------------------------------------
