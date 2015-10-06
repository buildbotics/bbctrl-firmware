/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2015, Cauldron Development LLC
                 Copyright (c) 2003-2015, Stanford University
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

#include "Scanner.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

using namespace cb;
using namespace std;


int Scanner::defaultWS[6] = {0x20, 0x09, 0x0d, 0x0a, 0xfeff, 0};


Scanner::Scanner(const InputSource &source) : x(-2), source(source) {
  location.setCol(-1);
  location.setLine(1);
  if (!source.getName().empty()) location.setFilename(source.getName());
}


bool Scanner::hasMore() {
  if (x == -2) advance();
  return x != -1;
}


int Scanner::peek() {
  return hasMore() ? x : -1;
}


void Scanner::advance() {
  x = next();

  switch (x) {
  case '\n':
    location.setCol(0);
    location.incLine();
    break;

  case '\r': break; // Ignore

  default:
    location.incCol();
    break;
  }
}


void Scanner::match(int c) {
  if (!hasMore())  THROWS("Expected '" << String::escapeC(c)
                          << "' found end of stream");
  if (c != peek()) THROWS("Expected '" << String::escapeC(c)
                          << "' found '" << String::escapeC(peek()));

  advance();
}


bool Scanner::consume(int c) {
  if (c == x) {
    advance();
    return true;
  }

  return false;
}


string Scanner::seek(const int *s, bool inverse, bool skip) {
  string buffer;

  while (hasMore()) {
    const int *ptr;
    for (ptr = s; *ptr; ptr++)
      if (*ptr == x) break;

    if ((inverse && !*ptr) || (!inverse && *ptr)) break;

    if (!skip) buffer += (char)x; // TODO does not work with UTF-8
    advance();
  }

  return buffer;
}


void Scanner::skipWhiteSpace(const int *s) {
  seek(s, true, true);
}


int Scanner::next() {
  istream &stream = source.getStream();

  if (!stream.good()) return -1;

  int x = stream.get();

  // Handle UTF-8
  int width = 0;
  if ((x & 0xe0) == 0xc0) width = 1;
  else if ((x & 0xf0) == 0xe0) width = 2;
  else if ((x & 0xf8) == 0xf0) width = 3;

  if (width) {
    bool extractedChars = false;
    uint32_t utf8 = x & ((1 << (6 - width)) - 1);

    for (int i = 0; i < width && stream.good(); i++) {
      int c = stream.peek();

      if ((c & 0xc0) != 0x80) break; // Not UTF-8

      utf8 = (utf8 << 6) | (c & 0x3f);

      stream.ignore();
      extractedChars = true;

      if (i == width - 1) return utf8;
    }

    if (extractedChars) THROW("Invalid UTF-8 data");
  }

  return x;
}
