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

#include "Writer.h"

#include <cbang/String.h>

using namespace std;
using namespace cb::JSON;


void Writer::close() {
  NullSync::close();
  stream.flush();
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
  stream << '"' << String::escapeC(value) << '"';
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
