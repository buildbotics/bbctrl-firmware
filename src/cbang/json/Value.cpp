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

#include "Value.h"

#include "Undefined.h"
#include "Null.h"
#include "Number.h"
#include "Boolean.h"
#include "String.h"

#include <sstream>
#include <limits>

using namespace std;
using namespace cb::JSON;


void Value::appendUndefined() {append(Undefined::instancePtr());}
void Value::appendNull() {append(Null::instancePtr());}
void Value::appendBoolean(bool value) {append(new Boolean(value));}
void Value::append(double value) {append(new Number(value));}
void Value::append(const std::string &value) {append(new String(value));}


int32_t Value::getS32() const {
  double value = getNumber();

  if (value < numeric_limits<int32_t>::min() ||
      numeric_limits<int32_t>::max() < value)
    CBANG_THROW("Value is not a 32-bit signed integer");

  return (int32_t)value;
}


uint32_t Value::getU32() const {
  double value = getNumber();

  if (value < numeric_limits<uint32_t>::min() ||
      numeric_limits<uint32_t>::max() < value)
    CBANG_THROW("Value is not a 32-bit unsigned integer");

  return (uint32_t)value;
}


int64_t Value::getS64() const {
  if (isString()) return cb::String::parseS64(getString());
  else {
    double value = getNumber();

    if (value < numeric_limits<int64_t>::min() ||
        numeric_limits<int64_t>::max() < value)
      CBANG_THROW("Value is not a 64-bit signed integer");

    return value;
  }
}


uint64_t Value::getU64() const {
  if (isString()) return cb::String::parseU64(getString());
  else {
    double value = getNumber();

    if (value < numeric_limits<uint64_t>::min() ||
        numeric_limits<uint64_t>::max() < value)
      CBANG_THROW("Value is not a 64-bit unsigned integer");

    return value;
  }
}


void Value::setUndefined(unsigned i) {set(i, Undefined::instancePtr());}
void Value::setNull(unsigned i) {set(i, Null::instancePtr());}
void Value::setBoolean(unsigned i, bool value) {set(i, new Boolean(value));}
void Value::set(unsigned i, double value) {set(i, new Number(value));}


void Value::set(unsigned i, const string &value) {
  set(i, new String(value));
}


void Value::insertUndefined(const string &key) {
  insert(key, Undefined::instancePtr());
}


void Value::insertNull(const string &key) {insert(key, Null::instancePtr());}


void Value::insertBoolean(const string &key, bool value) {
  insert(key, new Boolean(value));
}


void Value::insert(const string &key, double value) {
  insert(key, new Number(value));
}


void Value::insert(const string &key, const string &value) {
  insert(key, new String(value));
}


string Value::toString(unsigned indent, bool compact) const {
  ostringstream str;
  Writer writer(str, indent, compact);
  write(writer);
  str << flush;
  return str.str();
}
