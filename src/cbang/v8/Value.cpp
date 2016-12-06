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
#include "ValueRef.h"
#include "Factory.h"
#include "JSImpl.h"

#include <cbang/js/Callback.h>
#include <cbang/js/Sink.h>

using namespace cb::gv8;
using namespace cb;
using namespace std;


namespace {
  v8::Handle<v8::Value> _callback(const v8::Arguments &argv) {
    js::Callback &cb =
      *static_cast<js::Callback *>(v8::External::Cast(*argv.Data())->Value());

    try {
      // Convert args
      Value args = Value::createArray(argv.Length());
      for (int i = 0; i < argv.Length(); i++)
        args.set(i, Value(argv[i]));

      // Call function
      return cb.call(args).cast<Value>()->getV8Value();

    } catch (const Exception &e) {
      return v8::ThrowException(v8::String::New(SSTR(e).c_str()));

    } catch (const std::exception &e) {
      return v8::ThrowException(v8::String::New(e.what()));

    } catch (...) {
      return v8::ThrowException(v8::String::New("Unknown exception"));
    }
  }
}


Value::Value(const SmartPointer<js::Value> &value) :
  value(value.cast<Value>()->getV8Value()) {}


Value::Value(const js::Value &value) {
  const Value *v = dynamic_cast<const Value *>(&value);
  if (!v) THROW("Not a gv8::Value");
  this->value = v->getV8Value();
}


Value::Value(const js::Function &func) {
  SmartPointer<js::Callback> cb = func.getCallback();
  JSImpl::current().add(cb);

  v8::Handle<v8::Value> data = v8::External::New((void *)cb.get());
  v8::Handle<v8::FunctionTemplate> tmpl =
    v8::FunctionTemplate::New(&_callback, data);
  value = tmpl->GetFunction();
}


Value::Value(const Value &value) {
  if (value.getV8Value().IsEmpty()) this->value = v8::Undefined();
  else this->value = value.getV8Value();
}


Value::Value(const v8::Handle<v8::Value> &value) {
  if (value.IsEmpty()) this->value = v8::Undefined();
  else this->value = value;
}


Value::Value(const char *s, int length) :
  value(v8::String::New(s, length)) {}


Value::Value(const std::string &s) :
  value(v8::String::New(s.data(), s.length())) {}


SmartPointer<js::Value> Value::makePersistent() const {
  return new ValueRef(*this);
}


string Value::toString() const {
  if (isUndefined()) return "undefined";
  // TODO this is not very efficient
  v8::String::Utf8Value s(value);
  return *s ? string(*s, s.length()) : "(null)";
}


bool Value::has(const string &key) const {
  return value->ToObject()->
    Has(v8::String::NewSymbol(key.data(), key.length()));
}


SmartPointer<js::Value> Value::get(int index) const {
  return new Value(value->ToObject()->Get(index));
}


SmartPointer<js::Value> Value::get(const string &key) const {
  return new Value
    (value->ToObject()->
     Get(v8::String::NewSymbol(key.data(), key.length())));
}


SmartPointer<js::Value> Value::getOwnPropertyNames() const {
  return new Value(value->ToObject()->GetOwnPropertyNames());
}


void Value::set(int index, const js::Value &value) {
  this->value->ToObject()->Set(index, Value(value).getV8Value());
}


void Value::set(const string &key, const js::Value &value) {
  this->value->ToObject()->
    Set(v8::String::NewSymbol(key.data(), key.length()),
        Value(value).getV8Value());
}


unsigned Value::length() const {
  if (isString()) return v8::String::Cast(*value)->Length();
  else if (isArray()) return v8::Array::Cast(*value)->Length();
  else if (isObject())
    return value->ToObject()->GetOwnPropertyNames()->Length();
  THROW("Value does not have length");
}


Value Value::call(Value arg0, vector<Value> args) {
  if (!isFunction()) THROWS("Value is not a function");

  SmartPointer<v8::Handle<v8::Value> >::Array argv =
    new v8::Handle<v8::Value>[args.size()];

  for (unsigned i = 0; i < args.size(); i++)
    argv[i] = args[i].getV8Value();

  return v8::Handle<v8::Function>::Cast(value)->
    Call(arg0.getV8Value()->ToObject(), args.size(), argv.get());

  return 0;
}


string Value::getName() const {
  if (!isFunction()) THROWS("Value is not a function");
  return Value(v8::Handle<v8::Function>::Cast(value)->GetName()).
    toString();
}


void Value::setName(const string &name) {
  if (!isFunction()) THROWS("Value is not a function");
  v8::Handle<v8::Function>::Cast(value)->
    SetName(v8::String::NewSymbol(name.c_str(), name.length()));
}


int Value::getScriptLineNumber() const {
  if (!isFunction()) THROWS("Value is not a function");
  return v8::Handle<v8::Function>::Cast(value)->GetScriptLineNumber();
}
