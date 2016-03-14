/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

/* Supporting functions including:
 *
 *    - math and min/max utilities and extensions
 *    - vector manipulation utilities
 *    - support for debugging routines
 */

#pragma once


#include "config.h"

#include <stdint.h>
#include <math.h>

// Vector utilities
extern float vector[AXES]; // vector of axes for passing to subroutines

#define clear_vector(a) memset(a, 0, sizeof(a))
#define copy_vector(d,s) memcpy(d, s, sizeof(d))

float get_axis_vector_length(const float a[], const float b[]);
uint8_t vector_equal(const float a[], const float b[]);
float *set_vector(float x, float y, float z, float a, float b, float c);
float *set_vector_by_axis(float value, uint8_t axis);

// Math utilities
float min3(float x1, float x2, float x3);
float min4(float x1, float x2, float x3, float x4);
float max3(float x1, float x2, float x3);
float max4(float x1, float x2, float x3, float x4);


// String utilities
uint8_t isnumber(char c);
char *escape_string(char *dst, char *src);
char fntoa(char *str, float n, uint8_t precision);
uint16_t compute_checksum(char const *string, const uint16_t length);


// side-effect safe forms of min and max
#ifndef max
#define max(a,b)                                \
  ({ __typeof__ (a) termA = (a);                \
    __typeof__ (b) termB = (b);                 \
    termA>termB ? termA:termB; })
#endif

#ifndef min
#define min(a,b)                                \
  ({ __typeof__ (a) term1 = (a);                \
    __typeof__ (b) term2 = (b);                 \
    term1<term2 ? term1:term2; })
#endif

#ifndef avg
#define avg(a,b) ((a+b)/2)
#endif

// allowable rounding error for floats
#ifndef EPSILON
#define EPSILON        ((float)0.00001)
#endif

#ifndef fp_EQ
#define fp_EQ(a,b) (fabs(a - b) < EPSILON)
#endif
#ifndef fp_NE
#define fp_NE(a,b) (fabs(a - b) > EPSILON)
#endif
#ifndef fp_ZERO
#define fp_ZERO(a) (fabs(a) < EPSILON)
#endif
#ifndef fp_NOT_ZERO
#define fp_NOT_ZERO(a) (fabs(a) > EPSILON)
#endif
/// float is interpreted as FALSE (equals zero)
#ifndef fp_FALSE
#define fp_FALSE(a) (a < EPSILON)
#endif
/// float is interpreted as TRUE (not equal to zero)
#ifndef fp_TRUE
#define fp_TRUE(a) (a > EPSILON)
#endif

// Constants
#define MAX_LONG (2147483647)
#define MAX_ULONG (4294967295)
#define MM_PER_INCH (25.4)
#define INCHES_PER_MM (1 / 25.4)
#define MICROSECONDS_PER_MINUTE ((float)60000000)
#define uSec(a) ((float)(a * MICROSECONDS_PER_MINUTE))

#define RADIAN (57.2957795)
#ifndef M_SQRT3
#define M_SQRT3 (1.73205080756888)
#endif
