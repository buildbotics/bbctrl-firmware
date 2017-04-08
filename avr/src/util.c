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
float invsqrt(float x) {
  // evil floating point bit level hacking
  union {
    float f;
    int32_t i;
  } u;

  const float xhalf = x * 0.5f;
  u.f = x;
  u.i = 0x5f3759df - (u.i >> 1);          // what the fuck?
  u.f = u.f * (1.5f - xhalf * u.f * u.f); // 1st iteration
  u.f = u.f * (1.5f - xhalf * u.f * u.f); // 2nd iteration, can be removed

  return u.f;
}
