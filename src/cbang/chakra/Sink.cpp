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

#include "Sink.h"

using namespace cb::chakra;
using namespace std;


Sink::Sink(const Value &root) : root(root), closeList(false), closeDict(false) {
  if (root.isArray() || root.isObject()) {
    if (root.isArray()) {
      js::Sink::beginList();
      closeList = true;

    } else {
      js::Sink::beginDict();
      closeDict = true;
    }

    stack.push_back(root);
  }
}


void Sink::close() {
  if (closeList) endList();
  else if (closeDict) endDict();
  js::Sink::close();
}


void Sink::reset() {
  js::Sink::reset();
  stack.clear();
  root = Value::getUndefined();
}


void Sink::writeNull() {
  js::Sink::writeNull();
  write(Value::getNull());
}


void Sink::writeBoolean(bool value) {
  js::Sink::writeBoolean(value);
  write(Value(value));
}


void Sink::write(double value) {
  js::Sink::write(value);
  write(Value(value));
}


void Sink::write(const string &value) {
  js::Sink::write(value);
  write(Value(value));
}


void Sink::write(const js::Function &func) {
  js::Sink::writeNull();
  write(Value(func));
}


void Sink::write(const Value &value) {
  if (inList()) stack.back().set(index, value);
  else if (inDict()) stack.back().set(key, value);
  else root = value;
}


void Sink::beginList(bool simple) {
  Value value = Value::createArray();
  write(value);
  stack.push_back(value);
  js::Sink::beginList(simple);
}


void Sink::beginAppend() {
  js::Sink::beginAppend();
  index = stack.back().length();
}


void Sink::endList() {
  js::Sink::endList();
  stack.pop_back();
}


void Sink::beginDict(bool simple) {
  Value value = Value::createObject();
  write(value);
  stack.push_back(value);
  js::Sink::beginDict(simple);
}


void Sink::beginInsert(const string &key) {
  js::Sink::beginInsert(key);
  this->key = key;
}


void Sink::endDict() {
  js::Sink::endDict();
  stack.pop_back();
}
