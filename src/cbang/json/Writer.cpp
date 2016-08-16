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
  NullSink::close();
  stream.flush();
}


void Writer::reset() {
  NullSink::reset();
  stream.flush();
  level = initLevel;
  simple = false;
  first = true;
}


void Writer::writeNull() {
  NullSink::writeNull();
  stream << (mode == PYTHON_MODE ? "None" : "null");
}


void Writer::writeBoolean(bool value) {
  NullSink::writeBoolean(value);
  stream << (mode == PYTHON_MODE ?
             (value ? "True" : "False") : (value ? "true" : "false"));
}


void Writer::write(double value) {
  NullSink::write(value);
  stream << cb::String(value);
}


void Writer::write(const string &value) {
  NullSink::write(value);
  stream << '"' << escape(value) << '"';
}


void Writer::beginList(bool simple) {
  NullSink::beginList(simple);
  this->simple = simple;
  stream << "[";
  level++;
  first = true;
}


void Writer::beginAppend() {
  NullSink::beginAppend();

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
  NullSink::endList();
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
  NullSink::beginDict(simple);
  this->simple = simple;
  stream << "{";
  level++;
  first = true;
}


void Writer::beginInsert(const string &key) {
  NullSink::beginInsert(key);
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
  NullSink::endDict();
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
        else {
          // Invalid or non-standard UTF-8 code width, escape it
          result.append(String::printf("\\u%04x", (unsigned)c));
          break;
        }

        // Check if UTF-8 code is valid
        bool valid = true;
        uint32_t code = c & 0x1f;
        string data = string(1, c);

        for (int i = 0; i < width; i++) {
          // Check for early end of string
          if (++it == s.end()) {valid = false; break;}

          data.push_back(*it);

          // Check for invalid start bits
          if ((*it & 0xc0) != 0x80) {valid = false; break;}

          code = (code << 6) | (*it & 0x3f);
        }

        if (valid) {
          // Always encode Javascript line separators
          if (code < 0x2000 || 0x2100 < code) String::printf("\\u%04x", code);
          else result.append(data); // Otherwise, pass valid UTF-8

        } else {
          // Encode start character
          result.append(String::printf("\\u%04x",
                                       (unsigned)(unsigned char)data[0]));

          // Rewind
          it -= data.length() - 1;
        }

      } else if (iscntrl(c)) // Always encode control characters
        result.append(String::printf("\\u%04x", (unsigned)c));

      else result.push_back(c);
      break;
    }
  }

  return result;
}
