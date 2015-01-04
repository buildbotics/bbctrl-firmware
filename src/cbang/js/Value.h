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

#ifndef CB_JS_VALUE_H
#define CB_JS_VALUE_H

#include <cbang/StdTypes.h>

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
      ValueBase();
      ValueBase(const ValueBase<> &value);
      ValueBase(const ValueBase<v8::Persistent<v8::Value> > &value);
      ValueBase(const value_t &value);
      explicit ValueBase(bool x);
      ValueBase(double x);
      ValueBase(int32_t x);
      ValueBase(uint32_t x);
      ValueBase(const char *s, int length = -1);
      ValueBase(const std::string &s);

      // Undefined
      void assertDefined() const;
      bool isUndefined() const {return value->IsUndefined();}

      // Null
      bool isNull() const {return value->IsNull();}

      // Boolean
      void assertBoolean() const;
      bool isBoolean() const {return value->IsBoolean();}
      bool toBoolean() const {assertDefined(); return value->BooleanValue();}

      // Number
      void assertNumber() const;
      bool isNumber() const {return value->IsNumber();}
      double toNumber() const {assertDefined(); return value->NumberValue();}
      int64_t toInteger() const {assertDefined(); return value->IntegerValue();}

      // Int32
      void assertInt32() const;
      bool isInt32() const {return value->IsInt32();}
      int32_t toInt32() const {assertDefined(); return value->Int32Value();}

      // Uint32
      void assertUint32() const;
      bool isUint32() const {return value->IsUint32();}
      uint32_t toUint32() const {assertDefined(); return value->Uint32Value();}

      // String
      void assertString() const;
      bool isString() const {return value->IsString();}
      std::string toString() const;
      int utf8Length() const {return v8::String::Cast(*value)->Utf8Length();}
      void print(std::ostream &stream) const;

      // Object
      static Value createObject();
      void assertObject() const;
      bool isObject() const {return value->IsObject();}
      bool has(uint32_t index) const;
      bool has(const std::string &key) const;
      Value get(uint32_t index) const;
      Value get(const std::string &key) const;
      Value get(uint32_t index, Value defaultValue) const;
      Value get(const std::string &key, Value defaultValue) const;
      Value getOwnPropertyNames() const;
      void set(uint32_t index, Value value);
      void set(const std::string &key, Value value);

      // Array
      static Value createArray(unsigned size = 0);
      void assertArray() const;
      bool isArray() const {return value->IsArray();}
      int length() const;

      // Function
      bool isFunction() const {return value->IsFunction();}
      Value call(Value arg0);
      Value call(Value arg0, Value arg1);
      Value call(Value arg0, Value arg1, Value arg2);
      Value call(Value arg0, Value arg1, Value arg2, Value arg3);
      Value call(Value arg0, std::vector<Value> args);
      std::string getName() const;
      void setName(const std::string &name);
      int getScriptLineNumber() const;

      // Accessors
      const value_t &getV8Value() const {return value;}
      value_t &getV8Value() {return value;}
    };


    inline static
    std::ostream &operator<<(std::ostream &stream, const Value &v) {
      v.print(stream);
      return stream;
    }
  }
}

#endif // CB_JS_VALUE_H
