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

#ifndef CB_CHAKRA_VALUE_H
#define CB_CHAKRA_VALUE_H

#include <cbang/Exception.h>
#include <cbang/js/Value.h>
#include <cbang/js/Function.h>

#include <string>


namespace cb {
  namespace chakra {
    extern "C" typedef void *(*callback_t)
      (void *, bool, void **, unsigned short, void *);


    class Value : public js::Value {
    protected:
      void *ref;

    public:
      Value(void *ref = 0) : ref(ref) {}
      Value(const SmartPointer<js::Value> &value);
      Value(const std::string &value);
      Value(int value);
      explicit Value(double value);
      explicit Value(bool value);
      Value(const js::Function &func);
      Value(const std::string &name, callback_t cb, void *data);

      int getType() const;

      // From js::Value
      bool isArray() const;
      bool isBoolean() const;
      bool isFunction() const;
      bool isNull() const;
      bool isNumber() const;
      bool isObject() const;
      bool isString() const;
      bool isUndefined() const;

      bool toBoolean() const;
      int toInteger() const;
      double toNumber() const;
      std::string toString() const;

      unsigned length() const;
      SmartPointer<js::Value> get(int i) const;

      bool has(const std::string &key) const;
      SmartPointer<js::Value> get(const std::string &key) const;


      void set(int i, const Value &value);
      void append(const Value &value);
      void set(const std::string &key, const Value &value, bool strict = false);
      Value call(std::vector<Value> args) const;

      Value getOwnPropertyNames() const;
      void copyProperties(const Value &value);

      void setNull(const std::string &key);
      Value setObject(const std::string &key);
      Value setArray(const std::string &key, unsigned length = 0);

      operator void * () const {return ref;}

      static Value getGlobal();
      static Value getNull();
      static Value getUndefined();

      static bool hasException();
      static Value getException();

      static Value createArray(unsigned length = 0);
      static Value createArrayBuffer(unsigned length = 0, const char *data = 0);
      static Value createArrayBuffer(const std::string &s);
      static Value createObject();
      static Value createError(const std::string &msg);
      static Value createSyntaxError(const std::string &msg);

      static const char *errorToString(int error);
    };
  }
}


#define CHAKRA_CHECK(CMD)                                           \
  do {                                                              \
    JsErrorCode err = CMD;                                          \
    if (err != JsNoError)                                           \
      THROWS(#CMD << " failed with 0x" << std::hex << err           \
             << ' ' << Value::errorToString(err));                  \
  } while (0)


#endif // CB_CHAKRA_VALUE_H
