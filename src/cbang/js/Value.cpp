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


// Forward declaration of template specialization
namespace cb {
  namespace js {
    template<>
    ValueBase<v8::Persistent<v8::Value> >::ValueBase(const ValueBase<> &);
    template<>
    ValueBase<v8::Persistent<v8::Value> >::ValueBase(const value_t &);
  }
}


// Explicit instantiations
template class ValueBase<>;
template class ValueBase<v8::Persistent<v8::Value> >;


template<typename T>
ValueBase<T>::ValueBase() : value(v8::Undefined()) {}


template<typename T>
ValueBase<T>::ValueBase(const Value &value) :
value(value.getV8Value().IsEmpty() ? value_t(v8::Undefined()) :
      value.getV8Value()) {}


template<>
ValueBase<v8::Persistent<v8::Value> >::ValueBase(const Value &value) :
value(value.getV8Value().IsEmpty() ? value_t(v8::Undefined()) :
      value_t::New(value.getV8Value())) {}


template<typename T>
ValueBase<T>::ValueBase(const PersistentValue &value) :
value(value.getV8Value().IsEmpty() ? value_t(v8::Undefined()) :
      value_t(value.getV8Value())) {}


template<typename T>
ValueBase<T>::ValueBase(const value_t &value) :
  value(value.IsEmpty() ? value_t(v8::Undefined()) : value) {}


template<>
ValueBase<v8::Persistent<v8::Value> >::ValueBase(const value_t &value) :
value(value.IsEmpty() ? value_t(v8::Undefined()) : value_t::New(value)) {}


template<typename T>
ValueBase<T>::ValueBase(bool x) : value(x ? v8::True() : v8::False()) {}


template<typename T>
ValueBase<T>::ValueBase(double x) : value(v8::Number::New(x)) {}


template<typename T>
ValueBase<T>::ValueBase(int32_t x) : value(v8::Int32::New(x)) {}


template<typename T>
ValueBase<T>::ValueBase(uint32_t x) : value(v8::Uint32::New(x)) {}


template<typename T>
ValueBase<T>::ValueBase(const char *s, int length) :
  value(v8::String::New(s, length)) {}


template<typename T>
ValueBase<T>::ValueBase(const string &s) :
value(v8::String::New(s.data(), s.length())) {}


template<typename T>
void ValueBase<T>::assertDefined() const {
  if (isUndefined()) THROW("Value is undefined");
}


template<typename T>
void ValueBase<T>::assertBoolean() const {
  if (!isBoolean()) THROW("Value is not a boolean");
}


template<typename T>
void ValueBase<T>::assertNumber() const {
  if (!isNumber()) THROW("Value is not a number");
}


template<typename T>
void ValueBase<T>::assertInt32() const {
  if (!isInt32()) THROW("Value is not a int32");
}


template<typename T>
void ValueBase<T>::assertUint32() const {
  if (!isUint32()) THROW("Value is not a uint32");
}


template<typename T>
void ValueBase<T>::assertString() const {
  if (!isString()) THROW("Value is not a string");
}


template<typename T>
string ValueBase<T>::toString() const {
  assertDefined();
  // TODO this is not very efficient
  v8::String::Utf8Value s(value);
  return *s ? string(*s, s.length()) : "(null)";
}


template<typename T>
void ValueBase<T>::print(ostream &stream) const {
  v8::String::Utf8Value s(value);
  stream.write(*s, s.length());
}


template<typename T>
Value ValueBase<T>::createObject() {
  return T(v8::Object::New());
}


template<typename T>
void ValueBase<T>::assertObject() const {
  if (!isObject()) THROW("Value is not a object");
}


template<typename T>
bool ValueBase<T>::has(uint32_t index) const {
  return value->ToObject()->Has(index);
}


template<typename T>
bool ValueBase<T>::has(const string &key) const {
  return
    value->ToObject()->Has(v8::String::NewSymbol(key.data(), key.length()));
}


template<typename T>
Value ValueBase<T>::get(uint32_t index) const {
  return value->ToObject()->Get(index);
}


template<typename T>
Value ValueBase<T>::get(const string &key) const {
  return
    value->ToObject()->Get(v8::String::NewSymbol(key.data(), key.length()));
}


template<typename T>
Value ValueBase<T>::get(uint32_t index, Value defaultValue) const {
  return has(index) ? get(index) : defaultValue;
}


template<typename T>
Value ValueBase<T>::get(const string &key, Value defaultValue) const {
  return has(key) ? get(key) : defaultValue;
}


template<typename T>
Value ValueBase<T>::getOwnPropertyNames() const {
  return T(value->ToObject()->GetOwnPropertyNames());
}


template<typename T>
void ValueBase<T>::set(uint32_t index, Value value) {
  this->value->ToObject()->Set(index, value.getV8Value());
}


template<typename T>
void ValueBase<T>::set(const string &key, Value value) {
  this->value->ToObject()->Set(v8::String::NewSymbol(key.data(), key.length()),
                               value.getV8Value());
}


template<typename T>
Value ValueBase<T>::createArray(unsigned size) {
  return T(v8::Array::New(size));
}


template<typename T>
void ValueBase<T>::assertArray() const {
  if (!isArray()) THROW("Value is not a array");
}


template<typename T>
int ValueBase<T>::length() const {
  if (isString()) return v8::String::Cast(*value)->Length();
  else if (isArray()) return v8::Array::Cast(*value)->Length();
  else if (isObject())
    return value->ToObject()->GetOwnPropertyNames()->Length();
  THROW("Value does not have length");
}
