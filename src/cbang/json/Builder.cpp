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

#include "Builder.h"
#include "JSON.h"

using namespace std;
using namespace cb;
using namespace cb::JSON;


Builder::Builder(const ValuePtr &root) : appendNext(false) {
  if (!root.isNull()) stack.push_back(root);
}


ValuePtr Builder::getRoot() const {
  return stack.empty() ? 0 : stack.front();
}


void Builder::writeNull() {
  add(Null::instancePtr());
}


void Builder::writeBoolean(bool value) {
  add(new Boolean(value));
}


void Builder::write(double value) {
  add(new Number(value));
}


void Builder::write(const string &value) {
  add(new String(value));
}


void Builder::beginList(bool simple) {
  add(new List);
}


void Builder::beginAppend() {
  assertNotPending();
  appendNext = true;
}


void Builder::endList() {
  assertNotPending();

  if (stack.empty() || !stack.back()->isList()) THROW("Not a List");
  stack.pop_back();
}


void Builder::beginDict(bool simple) {
  add(new Dict);
}


void Builder::beginInsert(const string &key) {
  assertNotPending();
  nextKey = key;
}


void Builder::endDict() {
  assertNotPending();

  if (stack.empty() || !stack.back()->isDict()) THROW("Not a Dict");
  stack.pop_back();
}


void Builder::add(const ValuePtr &value) {
  if (shouldAppend()) stack.back()->append(value);

  else if (shouldInsert()) {
    stack.back()->insert(nextKey, value);
    nextKey.clear();

  } else if (!stack.empty()) THROWS("Cannot add " << value->getType());

  if (stack.empty() || value->isList() || value->isDict())
    stack.push_back(value);
}


void Builder::assertNotPending() {
  if (appendNext) THROW("Already called append()");
  if (!nextKey.empty()) THROW("Already called insert()");
}


bool Builder::shouldAppend() {
  if (appendNext) {
    appendNext = false;
    return true;

  } else return false;
}
