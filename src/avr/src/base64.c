/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "base64.h"

#include "util.h"

#include <ctype.h>


static const char *_b64_encode =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


static const int8_t _b64_decode[] = { // 43-122
                                              62, -1, -1, -1, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};


static int8_t _decode(char c) {
  return (c < 43 || 122 < c) ? -1 : _b64_decode[c - 43];
}


static char _encode(uint8_t b) {return _b64_encode[b & 63];}


static void _skip_space(const char **s, const char *end) {
  while (*s < end && isspace(**s)) (*s)++;
}


static char _next(const char **s, const char *end) {
  char c = *(*s)++;
  _skip_space(s, end);
  return c;
}


unsigned b64_encoded_length(unsigned len, bool pad) {
  unsigned elen = len / 3 * 4;

  switch (len % 3) {
  case 1: elen += pad ? 4 : 2; break;
  case 2: elen += pad ? 4 : 3; break;
  }

  return elen;
}


void b64_encode(const uint8_t *in, unsigned len, char *out, bool pad) {
  const uint8_t *end = in + len;
  int padding = 0;
  uint8_t a, b, c;

  while (in < end) {
    a = *in++;

    if (in < end) {
      b = *in++;

      if (in < end) c = *in++;
      else {c = 0; padding = 1;}

    } else {c = b = 0; padding = 2;}

    *out++ = _encode(63 & (a >> 2));
    *out++ = _encode(63 & (a << 4 | b >> 4));

    if (pad && padding == 2) *out++ = '=';
    else *out++ = _encode(63 & (b << 2 | c >> 6));

    if (pad && padding) *out++ = '=';
    else *out++ = _encode(63 & c);
  }
}


bool b64_decode(const char *in, unsigned len, uint8_t *out) {
  const char *end = in + len;

  _skip_space(&in, end);

  while (in < end) {
    int8_t w = _decode(_next(&in, end));
    int8_t x = in < end ? _decode(_next(&in, end)) : -2;
    int8_t y = in < end ? _decode(_next(&in, end)) : -2;
    int8_t z = in < end ? _decode(_next(&in, end)) : -2;

    if (w == -2 || x == -2 || w == -1 || x == -1 || y == -1 || z == -1)
      return false;

    *out++ = (uint8_t)(w << 2 | x >> 4);
    if (y != -2) {
      *out++ = (uint8_t)(x << 4 | y >> 2);
      if (z != -2) *out++ = (uint8_t)(y << 6 | z);
    }
  }

  return true;
}


bool b64_decode_float(const char *s, float *f) {
  union {
    float f;
    uint8_t b[4];
  } u;

  if (!b64_decode(s, 6, u.b)) return false;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define XORSWAP(a, b) a ^= (b ^ (b ^= a)
  XORSWAP(u.b[0], u.b[3]);
  XORSWAP(u.b[1], u.b[2]);
#endif

  *f = u.f;

  return true;
}
