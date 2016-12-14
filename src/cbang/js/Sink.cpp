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

using namespace cb::js;
using namespace cb;
using namespace std;



Sink::Sink(const SmartPointer<Factory> &factory,
           const SmartPointer<Value> &root) : factory(factory) {reset(root);}

Sink::Sink(const SmartPointer<Factory> &factory,
           Value &root) : factory(factory) {
  reset(SmartPointer<Value>::Phony(&root));
}


void Sink::close() {
  if (root.isNull()) return;
  if (closeList) endList();
  else if (closeDict) endDict();
  JSON::NullSink::close();
}


void Sink::reset(const SmartPointer<Value> &root) {
  closeList = closeDict = false;
  JSON::NullSink::reset();
  stack.clear();
  this->root = root;

  if (!root.isNull() && (root->isArray() || root->isObject())) {
    if (root->isArray()) {
      JSON::NullSink::beginList();
      closeList = true;

    } else {
      JSON::NullSink::beginDict();
      closeDict = true;
    }

    stack.push_back(root);
  }
}


void Sink::writeNull() {
  JSON::NullSink::writeNull();
  write(factory->createNull());
}


void Sink::writeBoolean(bool value) {
  JSON::NullSink::writeBoolean(value);
  write(factory->createBoolean(value));
}


void Sink::write(double value) {
  JSON::NullSink::write(value);
  write(factory->create(value));
}


void Sink::write(const string &value) {
  JSON::NullSink::write(value);
  write(factory->create(value));
}


void Sink::write(const js::Function &func) {
  JSON::NullSink::writeNull();
  write(factory->create(func));
}


void Sink::write(const SmartPointer<Value> &value) {
  if (inList()) stack.back()->set(index, value);
  else if (inDict()) stack.back()->set(key, value);
  else root = value;
}


void Sink::beginList(bool simple) {
  SmartPointer<Value> value = factory->createArray();
  write(value);
  stack.push_back(value);
  JSON::NullSink::beginList(simple);
}


void Sink::beginAppend() {
  JSON::NullSink::beginAppend();
  index = stack.back()->length();
}


void Sink::endList() {
  JSON::NullSink::endList();
  stack.pop_back();
}


void Sink::beginDict(bool simple) {
  SmartPointer<Value> value = factory->createObject();
  write(value);
  stack.push_back(value);
  JSON::NullSink::beginDict(simple);
}


void Sink::beginInsert(const string &key) {
  JSON::NullSink::beginInsert(key);
  this->key = key;
}


void Sink::endDict() {
  JSON::NullSink::endDict();
  stack.pop_back();
}
