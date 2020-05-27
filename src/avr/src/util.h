/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#pragma once


#include "config.h"
#include "status.h"

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>


// Vector utilities
#define clear_vector(a) memset(a, 0, sizeof(a))
#define copy_vector(d, s) memcpy(d, s, sizeof(d))

// Math utilities
inline static float min(float a, float b) {return a < b ? a : b;}
inline static float max(float a, float b) {return a < b ? b : a;}
inline static float min3(float a, float b, float c) {return min(min(a, b), c);}
inline static float max3(float a, float b, float c) {return max(max(a, b), c);}
inline static float min4(float a, float b, float c, float d)
{return min(min(a, b), min(c, d));}
inline static float max4(float a, float b, float c, float d)
{return max(max(a, b), max(c, d));}

float invsqrt(float number);

#ifndef __AVR__
inline static float square(float x) {return x * x;}
#endif

// Floating-point utilities
#define EPSILON 0.00001 // allowable rounding error for floats
inline static bool fp_EQ(float a, float b) {return fabs(a - b) < EPSILON;}
inline static bool fp_NE(float a, float b) {return fabs(a - b) > EPSILON;}
inline static bool fp_ZERO(float a) {return fabs(a) < EPSILON;}
inline static bool fp_FALSE(float a) {return fp_ZERO(a);}
inline static bool fp_TRUE(float a) {return !fp_ZERO(a);}

int8_t decode_hex_nibble(char c);
bool decode_float(char **s, float *f);
stat_t decode_axes(char **cmd, float axes[AXES]);
void format_hex_buf(char *buf, const uint8_t *data, unsigned len);

// Constants
#define MM_PER_INCH 25.4
#define INCHES_PER_MM (1 / 25.4)
#define MICROSECONDS_PER_MINUTE 60000000.0

// 24bit integers
#ifdef __AVR__
typedef __int24 int24_t;
typedef __uint24 uint24_t;
#else
typedef int32_t int24_t;
typedef uint32_t uint24_t;
#endif
