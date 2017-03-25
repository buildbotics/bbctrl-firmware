/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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

#include "util.h"

#include <stdint.h>


/// Fast inverse square root originally from Quake III Arena code.  Original
/// comments left intact.
/// See: https://en.wikipedia.org/wiki/Fast_inverse_square_root
float invsqrt(float number) {
  const float threehalfs = 1.5F;

  float x2 = number * 0.5F;
  float y = number;
  int32_t i = *(int32_t *)&y;          // evil floating point bit level hacking
  i = 0x5f3759df - (i >> 1);           // what the fuck?
  y = *(float *)&i;
  y = y * (threehalfs - x2 * y * y);   // 1st iteration
  y = y * (threehalfs - x2 * y * y);   // 2nd iteration, this can be removed

  return y;
}
