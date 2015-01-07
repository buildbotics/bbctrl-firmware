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


Scanner::Scanner(const InputSource &source) : x(-2), source(source) {
  location.setCol(-1);
  location.setLine(0);
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


void Scanner::match(char c) {
  if (!hasMore())  THROWS("Expected '" << String::escapeC(string(1, c))
                          << "' found end of stream");
  if (c != peek()) THROWS("Expected '" << String::escapeC(string(1, c))
                          << "' found '" << String::escapeC(string(1, peek())));

  advance();
}


bool Scanner::consume(char c) {
  if (c == (char)x) {
    advance();
    return true;
  }

  return false;
}


string Scanner::seek(const char *s, bool inverse, bool skip) {
  string buffer;

  while (hasMore()) {
    const char *ptr;
    for (ptr = s; *ptr; ptr++)
      if (*ptr == (char)x) break;

    if ((inverse && !*ptr) || (!inverse && *ptr)) break;

    if (!skip) buffer += (char)x;
    advance();
  }

  return buffer;
}


void Scanner::skipWhiteSpace(const char *s) {
  seek(s, true, true);
}
