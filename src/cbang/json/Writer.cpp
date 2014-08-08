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
  assertWriteNotPending();
  if (!stack.empty()) THROWS("Writer closed with open " << stack.back());
  stream.flush();
}


void Writer::writeNull() {
  assertCanWrite();
  stream << (mode == PYTHON_MODE ? "None" : "null");
}


void Writer::writeBoolean(bool value) {
  assertCanWrite();
  stream << (mode == PYTHON_MODE ?
             (value ? "True" : "False") : (value ? "true" : "false"));
}


void Writer::write(double value) {
  assertCanWrite();
  stream << cb::String(value);
}


void Writer::write(const string &value) {
  assertCanWrite();
  stream << '"' << String::escapeC(value) << '"';
}


void Writer::beginList(bool simple) {
  assertCanWrite();
  this->simple = simple;
  stream << "[";
  stack.push_back(ValueType::JSON_LIST);
  level++;
  first = true;
  canWrite = false;
}


void Writer::beginAppend() {
  assertWriteNotPending();
  if (stack.back() != ValueType::JSON_LIST) THROW("Not a List");

  if (first) first = false;
  else {
    stream << ',';
    if (isCompact()) stream << ' ';
  }

  if (!isCompact()) {
    stream << '\n';
    indent();
  }

  canWrite = true;
}


void Writer::endList() {
  assertWriteNotPending();
  if (stack.empty() || stack.back() != ValueType::JSON_LIST)
    THROW("Not a List");

  stack.pop_back();
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
  assertCanWrite();
  this->simple = simple;
  stream << "{";
  stack.push_back(ValueType::JSON_DICT);
  keyStack.push_back(keys_t());
  level++;
  first = true;
  canWrite = false;
}


bool Writer::has(const string &key) const {
  if (stack.empty() || stack.back() != ValueType::JSON_DICT)
    THROW("Not a Dict");
  return keyStack.back().find(key) != keyStack.back().end();
}


void Writer::beginInsert(const string &key) {
  assertWriteNotPending();
  if (has(key)) THROWS("Key '" << key << "' already written to output");

  if (first) first = false;
  else {
    stream << ',';
    if (isCompact()) stream << ' ';
  }

  if (!isCompact()) {
    stream << '\n';
    indent();
  }

  canWrite = true;
  write(key);
  stream << ": ";

  keyStack.back().insert(key);

  canWrite = true;
}



void Writer::endDict() {
  assertWriteNotPending();
  if (stack.empty() || stack.back() != ValueType::JSON_DICT)
    THROW("Not a Dict");

  stack.pop_back();
  keyStack.pop_back();
  level--;

  if (!isCompact() && !first) {
    stream << '\n';
    indent();
  }

  stream << "}";

  first = false;
  simple = false;
}


void Writer::assertCanWrite() {
  if (!canWrite) THROW("Not ready for write");
  canWrite = false;
}


void Writer::assertWriteNotPending() {
  if (canWrite) THROW("Expected write");
}
