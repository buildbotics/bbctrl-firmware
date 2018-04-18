/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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

#include "base64.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


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


int8_t decode_hex_nibble(char c) {
  if ('0' <= c && c <= '9') return c - '0';
  if ('a' <= c && c <= 'f') return c - 'a' + 10;
  if ('A' <= c && c <= 'F') return c - 'A' + 10;
  return -1;
}


bool decode_float(char **s, float *f) {
  bool ok = b64_decode_float(*s, f) && isfinite(*f);
  *s += 6;
  return ok;
}


stat_t decode_axes(char **cmd, float axes[AXES]) {
  while (**cmd) {
    const char *names = "xyzabc";
    const char *match = strchr(names, **cmd);
    if (!match) break;
    char *s = *cmd + 1;
    if (!decode_float(&s, &axes[match - names])) return STAT_BAD_FLOAT;
    *cmd = s;
  }

  return STAT_OK;
}


// Assumes the caller provide format buffer length is @param len * 2 + 1.
void format_hex_buf(char *buf, const uint8_t *data, unsigned len) {
  uint8_t i;

  for (i = 0; i < len; i++)
    sprintf(buf + i * 2, "%02x", data[i]);

  buf[i * 2] = 0;
}


void print_vector(float v[4]) {
  printf_P(PSTR("%f %f %f %f\n"),
           (double)v[0], (double)v[1], (double)v[2], (double)v[3]);
}
