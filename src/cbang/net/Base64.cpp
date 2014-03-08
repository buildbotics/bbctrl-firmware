/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Base64.h"

#include <cbang/Exception.h>

#include <locale>


using namespace std;
using namespace cb;


const char *Base64::encodeTable =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

const char Base64::decodeTable[256] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


string Base64::encode(const string &s) const {
  return encode(s.c_str(), s.length());
}


string Base64::encode(const char *_s, unsigned length) const {
  const uint8_t *s = (uint8_t *)_s;
  const uint8_t *end = s + length;
  string result;
  unsigned size = length / 3 * 4 + 4;
  if (width) size += (size / width) * 2;
  result.reserve(size);

  unsigned col = 0;
  int padding = 0;
  uint8_t a, b, c;
  while (s != end) {
    a = *s++;
    if (s == end) {c = b = 0; padding = 2;}
    else {
      b = *s++;
      if (s == end) {c = 0; padding = 1;}
      else c = *s++;
    }

    result += encode(63 & (a >> 2));
    result += encode(63 & (a << 4 | b >> 4));
    if (pad && padding == 2) result += pad;
    else result += encode(63 & (b << 2 | c >> 6));
    if (pad && padding) result += pad;
    else result += encode(63 & c);

    if (s != end && width) {
      col += 4;
      if (col == width) {
        col = 0;
        result += "\r\n";
      }
    }
  }

  return result;
}


string Base64::decode(const string &s) const {
  string result;
  result.reserve(s.length() / 4 * 3 + 1);

  string::const_iterator it = s.begin();
  while (it != s.end() && isspace(*it)) it++;

  while (it != s.end()) {
    char w = decode(next(it, s.end()));
    char x = it == s.end() ? -2 : decode(next(it, s.end()));
    char y = it == s.end() ? -2 : decode(next(it, s.end()));
    char z = it == s.end() ? -2 : decode(next(it, s.end()));

    if (w == -1 || x == -1 || y == -1 || z == -1)
      THROWS("Invalid Base64 data at " << (it - s.begin()));

    result += (char)(w << 2 | x >> 4);
    if (y != -2) {
      result += (char)(x << 4 | y >> 2);
      if (z != -2) result += (char)(y << 6 | z);
    }
  }

  return result;
}


char Base64::next(string::const_iterator &it,
                  string::const_iterator end) {
  char c = *it++;
  while (it != end && isspace(*it)) it++;
  return c;
}


char Base64::encode(int x) const {
  if (x == 62) return a;
  if (x == 63) return b;
  return encodeTable[x];
}


int Base64::decode(char x) const {
  if (x == a) return 62;
  if (x == b) return 63;
  if (x == pad) return -2;
  return decodeTable[(int)x];
}
