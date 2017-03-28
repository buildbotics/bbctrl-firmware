/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                  Copyright (c) 2010 - 2014 Alden S. Hart, Jr.
                            All rights reserved.

     This file ("the software") is free software: you can redistribute it
     and/or modify it under the terms of the GNU General Public License,
      version 2 as published by the Free Software Foundation. You should
      have received a copy of the GNU General Public License, version 2
     along with the software. If not, see <http://www.gnu.org/licenses/>.

     The software is distributed in the hope that it will be useful, but
          WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
               Lesser General Public License for more details.

       You should have received a copy of the GNU Lesser General Public
                License along with the software.  If not, see
                       <http://www.gnu.org/licenses/>.

                For information regarding this software email:
                  "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#pragma once


#include "config.h"

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
