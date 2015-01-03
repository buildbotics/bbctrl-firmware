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

#include "Value.h"

#include <cbang/Exception.h>

using namespace cb::js;
using namespace std;


Value::Value() : value(v8::Undefined()) {}
Value::Value(const v8::Handle<v8::Value> &value) :
  value(value.IsEmpty() ? v8::Handle<v8::Value>(v8::Undefined()) : value) {}
Value::Value(bool x) : value(x ? v8::True() : v8::False()) {}
Value::Value(double x) : value(v8::Number::New(x)) {}
Value::Value(int32_t x) : value(v8::Int32::New(x)) {}
Value::Value(uint32_t x) : value(v8::Uint32::New(x)) {}
Value::Value(const char *s, int length) : value(v8::String::New(s, length)) {}
Value::Value(const string &s) : value(v8::String::New(s.data(), s.length())) {}

void Value::assertDefined() const {
  if (isUndefined()) THROW("Value is undefined");
}


void Value::assertBoolean() const {
  if (!isBoolean()) THROW("Value is not a boolean");
}


void Value::assertNumber() const {
  if (!isNumber()) THROW("Value is not a number");
}


void Value::assertInt32() const {
  if (!isInt32()) THROW("Value is not a int32");
}


void Value::assertUint32() const {
  if (!isUint32()) THROW("Value is not a uint32");
}


void Value::assertString() const {
  if (!isString()) THROW("Value is not a string");
}


string Value::toString() const {
  assertDefined();
  // TODO this is not very efficient
  v8::String::Utf8Value s(value);
  return *s ? string(*s, s.length()) : "(null)";
}


void Value::print(ostream &stream) const {
  v8::String::Utf8Value s(value);
  stream.write(*s, s.length());
}


Value Value::createObject() {
  return v8::Handle<v8::Value>(v8::Object::New());
}


void Value::assertObject() const {
  if (!isObject()) THROW("Value is not a object");
}


bool Value::has(uint32_t index) const {
  return value->ToObject()->Has(index);
}


bool Value::has(const string &key) const {
  return
    value->ToObject()->Has(v8::String::NewSymbol(key.data(), key.length()));
}


Value Value::get(uint32_t index) const {
  return value->ToObject()->Get(index);
}


Value Value::get(const string &key) const {
  return
    value->ToObject()->Get(v8::String::NewSymbol(key.data(), key.length()));
}


Value Value::get(uint32_t index, Value defaultValue) const {
  return has(index) ? get(index) : defaultValue;
}


Value Value::get(const string &key, Value defaultValue) const {
  return has(key) ? get(key) : defaultValue;
}


Value Value::getOwnPropertyNames() const {
  return v8::Handle<v8::Value>(value->ToObject()->GetOwnPropertyNames());
}


void Value::set(uint32_t index, Value value) {
  this->value->ToObject()->Set(index, value.getV8Value());
}


void Value::set(const string &key, Value value) {
  this->value->ToObject()->Set(v8::String::NewSymbol(key.data(), key.length()),
                               value.getV8Value());
}


Value Value::createArray(unsigned size) {
  return v8::Handle<v8::Value>(v8::Array::New(size));
}


void Value::assertArray() const {
  if (!isArray()) THROW("Value is not a array");
}


int Value::length() const {
  if (isString()) return v8::String::Cast(*value)->Length();
  else if (isArray()) return v8::Array::Cast(*value)->Length();
  else if (isObject())
    return value->ToObject()->GetOwnPropertyNames()->Length();
  THROW("Value does not have length");
}
