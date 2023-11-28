/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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


bool decode_hex_u16(char **s, uint16_t *x) {
  for (int i = 0; i < 4; i++) {
    int8_t n = decode_hex_nibble(*(*s)++);
    if (n < 0) return false;
    *x = *x << 4 | n;
  }

  return true;
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
