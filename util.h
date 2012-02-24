// Copyright (c) 2011 Robert Kooima.  All Rights Reserved.

#ifndef SCMTIFF_UTIL_H
#define SCMTIFF_UTIL_H

//------------------------------------------------------------------------------

void  normalize(float *);

void  mid2(float *, const float *, const float *);
void  mid4(float *, const float *, const float *,
                    const float *, const float *);

float lerp1(float, float, float);
float lerp2(float, float, float, float, float, float);

void  slerp1(float *, const float *, const float *, float);
void  slerp2(float *, const float *, const float *,
                      const float *, const float *, float, float);

int extcmp(const char *, const char *);

//------------------------------------------------------------------------------

#endif
