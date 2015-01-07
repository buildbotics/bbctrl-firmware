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

#include "Writer.h"

#include <cbang/String.h>

#include <cctype>

using namespace std;
using namespace cb::JSON;


void Writer::close() {
  NullSync::close();
  stream.flush();
}


void Writer::reset() {
  NullSync::reset();
  level = initLevel;
  simple = false;
  first = true;
}


void Writer::writeNull() {
  NullSync::writeNull();
  stream << (mode == PYTHON_MODE ? "None" : "null");
}


void Writer::writeBoolean(bool value) {
  NullSync::writeBoolean(value);
  stream << (mode == PYTHON_MODE ?
             (value ? "True" : "False") : (value ? "true" : "false"));
}


void Writer::write(double value) {
  NullSync::write(value);
  stream << cb::String(value);
}


void Writer::write(const string &value) {
  NullSync::write(value);
  stream << '"' << escape(value) << '"';
}


void Writer::beginList(bool simple) {
  NullSync::beginList(simple);
  this->simple = simple;
  stream << "[";
  level++;
  first = true;
}


void Writer::beginAppend() {
  NullSync::beginAppend();

  if (first) first = false;
  else {
    stream << ',';
    if (isCompact()) stream << ' ';
  }

  if (!isCompact()) {
    stream << '\n';
    indent();
  }
}


void Writer::endList() {
  NullSync::endList();
  level--;

  if (!isCompact() && !first) {
    stream << '\n';
    indent();
  }

  stream << "]";

  first = false;
  simple = false;
}


void Writer::beginDict(bool simple) {
  NullSync::beginDict(simple);
  this->simple = simple;
  stream << "{";
  level++;
  first = true;
}


void Writer::beginInsert(const string &key) {
  NullSync::beginInsert(key);
  if (first) first = false;
  else {
    stream << ',';
    if (isCompact()) stream << ' ';
  }

  if (!isCompact()) {
    stream << '\n';
    indent();
  }

  write(key);
  stream << ": ";

  canWrite = true;
}



void Writer::endDict() {
  NullSync::endDict();
  level--;

  if (!isCompact() && !first) {
    stream << '\n';
    indent();
  }

  stream << "}";

  first = false;
  simple = false;
}


string Writer::escape(const string &s) {
  string result;
  result.reserve(s.length());

  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    unsigned char c = *it;

    switch (c) {
    case 0: result.append("\\u0000"); break;
    case '\\': result.append("\\\\"); break;
    case '\"': result.append("\\\""); break;
    case '\b': result.append("\\b"); break;
    case '\f': result.append("\\f"); break;
    case '\n': result.append("\\n"); break;
    case '\r': result.append("\\r"); break;
    case '\t': result.append("\\t"); break;
    default:
      // Check UTF-8 encodings.
      //
      // UTF-8 code can be of the following formats:
      //
      //    Range in Hex   Binary representation
      //        0-7f       0xxxxxxx
      //       80-7ff      110xxxxx 10xxxxxx
      //      800-ffff     1110xxxx 10xxxxxx 10xxxxxx
      //    10000-1fffff   11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
      //   200000-3ffffff  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
      //  4000000-7fffffff 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
      //
      // The last two formats are non-standard.
      //
      // See: http://en.wikipedia.org/wiki/UTF-8

      if (0x80 <= c) {
        // Compute code width
        int width = 0;
        if ((c & 0xe0) == 0xc0) width = 1;
        else if ((c & 0xf0) == 0xe0) width = 2;
        else if ((c & 0xf8) == 0xf0) width = 3;
        else if ((c & 0xfc) == 0xf8) width = 4;
        else if ((c & 0xfe) == 0xfc) width = 5;
        else THROW("Invalid UTF-8");

        // Escape short codes and pass long ones
        uint16_t code = c & 0x1f;
        if (2 < width) result.push_back(c);

        for (int i = 0; i < width; i++) {
          if (++it == s.end() || (*it & 0xc0) != 0x80)
            THROWS("Invalid UTF-8");

          if (2 < width) result.push_back(*it);
          else code = (code << 6) | (*it & 0x3f);
        }

        if (width < 3)
          result.append(String::printf("\\u%04x", (unsigned)code));

      } else if (iscntrl(c)) {
        result.append(1, '\\');
        result.append(1, 'x');
        result.append(1, String::hexNibble(c >> 4));
        result.append(1, String::hexNibble(c));

      } else result.push_back(c);
      break;
    }
  }

  return result;
}
