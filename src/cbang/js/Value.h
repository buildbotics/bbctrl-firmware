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

#ifndef CB_JS_VALUE_H
#define CB_JS_VALUE_H

#include <cbang/StdTypes.h>
#include <cbang/Exception.h>

#include "V8.h"

#include <string>
#include <ostream>
#include <vector>


namespace cb {
  namespace js {
    // Forward Declarations
    template <typename T> class ValueBase;

    // Type Definitions
    typedef ValueBase<v8::Handle<v8::Value> > Value;
    typedef ValueBase<v8::Persistent<v8::Value> > PersistentValue;


    template <typename T = v8::Handle<v8::Value> >
    class ValueBase {
    protected:
      typedef ValueBase<T> Super_T;
      typedef T value_t;
      value_t value;

    public:
      ValueBase() : value(v8::Undefined()) {}

      ValueBase(const ValueBase<> &value) :
        value(value.getV8Value().IsEmpty() ? value_t(v8::Undefined()) :
              value.getV8Value()) {}

      ValueBase(const ValueBase<v8::Persistent<v8::Value> > &value) :
        value(value.getV8Value().IsEmpty() ? value_t(v8::Undefined()) :
              value_t(value.getV8Value())) {}

      ValueBase(const value_t &value) :
      value(value.IsEmpty() ? value_t(v8::Undefined()) : value) {}

      explicit ValueBase(bool x) : value(x ? v8::True() : v8::False()) {}

      ValueBase(double x) : value(v8::Number::New(x)) {}

      ValueBase(int32_t x) : value(v8::Int32::New(x)) {}

      ValueBase(uint32_t x) : value(v8::Uint32::New(x)) {}

      ValueBase(const char *s, int length = -1) :
        value(v8::String::New(s, length)) {}

      ValueBase(const std::string &s) :
        value(v8::String::New(s.data(), s.length())) {}

      // Undefined
      void assertDefined() const
      {if (isUndefined()) THROW("Value is undefined");}
      bool isUndefined() const {return value->IsUndefined();}

      // Null
      bool isNull() const {return value->IsNull();}

      // Boolean
      void assertBoolean() const
      {if (!isBoolean()) THROW("Value is not a boolean");}
      bool isBoolean() const {return value->IsBoolean();}
      bool toBoolean() const {assertDefined(); return value->BooleanValue();}

      // Number
      void assertNumber() const
      {if (!isNumber()) THROW("Value is not a number");}
      bool isNumber() const {return value->IsNumber();}
      double toNumber() const {assertDefined(); return value->NumberValue();}
      int64_t toInteger() const {assertDefined(); return value->IntegerValue();}

      // Int32
      void assertInt32() const
      {if (!isInt32()) THROW("Value is not a int32");}
      bool isInt32() const {return value->IsInt32();}
      int32_t toInt32() const {assertDefined(); return value->Int32Value();}

      // Uint32
      void assertUint32() const
      {if (!isUint32()) THROW("Value is not a uint32");}
      bool isUint32() const {return value->IsUint32();}
      uint32_t toUint32() const {assertDefined(); return value->Uint32Value();}

      // String
      void assertString() const
      {if (!isString()) THROW("Value is not a string");}
      bool isString() const {return value->IsString();}

      std::string toString() const {
        if (isUndefined()) return "undefined";
        // TODO this is not very efficient
        v8::String::Utf8Value s(value);
        return *s ? std::string(*s, s.length()) : "(null)";
      }

      int utf8Length() const {return v8::String::Cast(*value)->Utf8Length();}

      void print(std::ostream &stream) const {
        v8::String::Utf8Value s(value);
        stream.write(*s, s.length());
      }

      // Object
      static Value createObject() {return T(v8::Object::New());}
      void assertObject() const
      {if (!isObject()) THROW("Value is not a object");}
      bool isObject() const {return value->IsObject();}
      bool has(uint32_t index) const
      {return value->ToObject()->Has(index);}

      bool has(const std::string &key) const {
        return value->ToObject()->
          Has(v8::String::NewSymbol(key.data(), key.length()));
      }

      Value get(uint32_t index) const
      {return value->ToObject()->Get(index);}

      Value get(const std::string &key) const {
        return value->ToObject()->
          Get(v8::String::NewSymbol(key.data(), key.length()));
      }

      Value get(uint32_t index, Value defaultValue) const
      {return has(index) ? get(index) : defaultValue;}

      Value get(const std::string &key, Value defaultValue) const
      {return has(key) ? get(key) : defaultValue;}

      Value getOwnPropertyNames() const
      {return T(value->ToObject()->GetOwnPropertyNames());}

      void set(uint32_t index, Value value)
      {this->value->ToObject()->Set(index, value.getV8Value());}

      void set(const std::string &key, Value value) {
      this->value->ToObject()->
        Set(v8::String::NewSymbol(key.data(), key.length()),
            value.getV8Value());
      }

      // Array
      static Value createArray(unsigned size = 0)
      {return T(v8::Array::New(size));}

      void assertArray() const
      {if (!isArray()) THROW("Value is not a array");}

      bool isArray() const {return value->IsArray();}

      int length() const {
        if (isString()) return v8::String::Cast(*value)->Length();
        else if (isArray()) return v8::Array::Cast(*value)->Length();
        else if (isObject())
          return value->ToObject()->GetOwnPropertyNames()->Length();
        THROW("Value does not have length");
      }

      // Function
      bool isFunction() const {return value->IsFunction();}

      Value call(Value arg0) {
        if (!isFunction()) THROWS("Value is not a function");
        return v8::Handle<v8::Function>::Cast(value)->
          Call(arg0.getV8Value()->ToObject(), 0, 0);
      }

      Value call(Value arg0, Value arg1) {
        if (!isFunction()) THROWS("Value is not a function");
        return v8::Handle<v8::Function>::Cast(value)->
          Call(arg0.getV8Value()->ToObject(), 1, &arg1.getV8Value());
      }

      Value call(Value arg0, Value arg1, Value arg2) {
        if (!isFunction()) THROWS("Value is not a function");

        v8::Handle<v8::Value> argv[] = {arg1.getV8Value(), arg2.getV8Value()};

        return v8::Handle<v8::Function>::Cast(value)->
          Call(arg0.getV8Value()->ToObject(), 2, argv);
      }

      Value call(Value arg0, Value arg1, Value arg2, Value arg3) {
        if (!isFunction()) THROWS("Value is not a function");

        v8::Handle<v8::Value> argv[] =
          {arg1.getV8Value(), arg2.getV8Value(), arg3.getV8Value()};

        return v8::Handle<v8::Function>::Cast(value)->
          Call(arg0.getV8Value()->ToObject(), 3, argv);
      }

      Value call(Value arg0, std::vector<Value> args) {
        if (!isFunction()) THROWS("Value is not a function");

        SmartPointer<v8::Handle<v8::Value> >::Array argv =
          new v8::Handle<v8::Value>[args.size()];

        for (unsigned i = 0; i < args.size(); i++)
          argv[i] = args[i].getV8Value();

        return v8::Handle<v8::Function>::Cast(value)->
          Call(arg0.getV8Value()->ToObject(), args.size(), argv.get());

        return 0;
      }

      std::string getName() const {
        if (!isFunction()) THROWS("Value is not a function");
        return Value(v8::Handle<v8::Function>::Cast(value)->GetName()).
          toString();
      }

      void setName(const std::string &name) {
        if (!isFunction()) THROWS("Value is not a function");
        v8::Handle<v8::Function>::Cast(value)->
          SetName(v8::String::NewSymbol(name.c_str(), name.length()));
      }

      int getScriptLineNumber() const {
        if (!isFunction()) THROWS("Value is not a function");
        return v8::Handle<v8::Function>::Cast(value)->GetScriptLineNumber();
      }

      // Accessors
      const value_t &getV8Value() const {return value;}
      value_t &getV8Value() {return value;}
    };




    // Stream operator
    inline static
    std::ostream &operator<<(std::ostream &stream, const Value &v) {
      v.print(stream);
      return stream;
    }
  }
}

#endif // CB_JS_VALUE_H
