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

#include <cbang/SmartPointer.h>


namespace cb {
  namespace js {
    class Value {
    public:
      virtual ~Value() {}

      virtual bool isArray() const {return false;}
      virtual bool isBoolean() const {return false;}
      virtual bool isFunction() const {return false;}
      virtual bool isNull() const {return false;}
      virtual bool isNumber() const {return false;}
      virtual bool isObject() const {return false;}
      virtual bool isString() const {return false;}
      virtual bool isUndefined() const {return false;}

      virtual bool toBoolean() const = 0;
      virtual int toInteger() const = 0;
      virtual double toNumber() const = 0;
      virtual std::string toString() const = 0;

      virtual unsigned length() const = 0;
      virtual SmartPointer<Value> get(int i) const = 0;

      virtual bool has(const std::string &key) const = 0;
      virtual SmartPointer<Value> get(const std::string &key) const = 0;

      // Array accessors
      bool getBoolean(int i) const {return get(i)->toBoolean();}
      int getInteger(int i) const {return get(i)->toInteger();}
      double getNumber(int i) const {return get(i)->toNumber();}
      std::string getString(int i) const {return get(i)->toString();}

      // Object accessors
      bool getBoolean(const std::string &key) const
      {return get(key)->toBoolean();}
      int getInteger(const std::string &key) const
      {return get(key)->toInteger();}
      double getNumber(const std::string &key) const
      {return get(key)->toNumber();}
      std::string getString(const std::string &key) const
      {return get(key)->toString();}
    };
  }
}

#endif // CB_JS_VALUE_H
