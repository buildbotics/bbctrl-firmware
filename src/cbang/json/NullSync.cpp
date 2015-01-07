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

#include "NullSync.h"

using namespace cb::JSON;


bool NullSync::inList() const {
  return !stack.empty() && stack.back() == ValueType::JSON_LIST;
}


bool NullSync::inDict() const {
  return !stack.empty() && stack.back() == ValueType::JSON_DICT;
}


void NullSync::end() {
  if (inList()) endList();
  else if (inDict()) endDict();
  else THROW("Not in list or dict");
}


void NullSync::close() {
  assertWriteNotPending();
  if (!stack.empty()) THROWS("Writer closed with open " << stack.back());
}


void NullSync::reset() {
  stack.clear();
  keyStack.clear();
  canWrite = true;
}


void NullSync::writeNull() {
  assertCanWrite();
}


void NullSync::writeBoolean(bool value) {
  assertCanWrite();
}


void NullSync::write(double value) {
  assertCanWrite();
}


void NullSync::write(const std::string &value) {
  assertCanWrite();
}


void NullSync::beginList(bool simple) {
  assertCanWrite();
  stack.push_back(ValueType::JSON_LIST);
  canWrite = false;
}


void NullSync::beginAppend() {
  assertWriteNotPending();
  if (!inList()) THROW("Not a List");
  canWrite = true;
}


void NullSync::endList() {
  assertWriteNotPending();
  if (!inList()) THROW("Not a List");

  stack.pop_back();
}


void NullSync::beginDict(bool simple) {
  assertCanWrite();
  stack.push_back(ValueType::JSON_DICT);
  keyStack.push_back(keys_t());
  canWrite = false;
}


bool NullSync::has(const std::string &key) const {
  if (!inDict()) THROW("Not a Dict");
  return keyStack.back().find(key) != keyStack.back().end();
}


void NullSync::beginInsert(const std::string &key) {
  assertWriteNotPending();
  if (has(key)) THROWS("Key '" << key << "' already written to output");
  keyStack.back().insert(key);
  canWrite = true;
}


void NullSync::endDict() {
  assertWriteNotPending();
  if (!inDict()) THROW("Not a Dict");

  stack.pop_back();
  keyStack.pop_back();
}


void NullSync::assertCanWrite() {
  if (!canWrite) THROW("Not ready for write");
  canWrite = false;
}


void NullSync::assertWriteNotPending() {
  if (canWrite) THROW("Expected write");
}
