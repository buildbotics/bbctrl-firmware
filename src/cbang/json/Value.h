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

#ifndef CBANG_JSON_VALUE_H
#define CBANG_JSON_VALUE_H

#include "ValueType.h"
#include "Writer.h"

#include <cbang/SmartPointer.h>
#include <cbang/Exception.h>
#include <cbang/StdTypes.h>

#include <ostream>

namespace cb {
  namespace JSON {
    class List;
    class Dict;
    class Null;
    class Value;

    typedef SmartPointer<Value> ValuePtr;

    class Value : public ValueType::Enum {
    public:
      virtual ~Value() {}

      virtual ValueType getType() const = 0;
      virtual ValuePtr copy(bool deep = false) const = 0;

      virtual bool isSimple() const {return true;}
      bool isNull() const {return getType() == JSON_NULL;}
      bool isBoolean() const {return getType() == JSON_BOOLEAN;}
      bool isNumber() const {return getType() == JSON_NUMBER;}
      bool isString() const {return getType() == JSON_STRING;}
      bool isList() const {return getType() == JSON_LIST;}
      bool isDict() const {return getType() == JSON_DICT;}

      virtual bool getBoolean() const {CBANG_THROW("Value is not a Boolean");}
      virtual double getNumber() const {CBANG_THROW("Value is not a Number");}
      virtual int32_t getS32() const;
      virtual uint32_t getU32() const;
      virtual std::string &getString() {CBANG_THROW("Value is not a String");}
      virtual const std::string &getString() const
      {CBANG_THROW("Value is not a String");}
      virtual List &getList() {CBANG_THROW("Value is not a List");}
      virtual const List &getList() const {CBANG_THROW("Value is not a List");}
      virtual Dict &getDict() {CBANG_THROW("Value is not a Dict");}
      virtual const Dict &getDict() const {CBANG_THROW("Value is not a Dict");}

      virtual unsigned size() const {CBANG_THROW("Not a List or Dict");}

      // List functions
      virtual const ValuePtr &get(unsigned i) const {CBANG_THROW("Not a List");}
      virtual void appendNull() {CBANG_THROW("Not a List");}
      virtual void appendBoolean(bool value) {CBANG_THROW("Not a List");}
      virtual void append(double value) {CBANG_THROW("Not a List");}
      virtual void append(const std::string &value) {CBANG_THROW("Not a List");}
      virtual void append(const ValuePtr &value) {CBANG_THROW("Not a List");}

      // List accessors
      bool getBoolean(unsigned i) const {return get(i)->getBoolean();}
      double getNumber(unsigned i) const {return get(i)->getNumber();}
      int32_t getS32(unsigned i) const {return get(i)->getS32();}
      uint32_t getU32(unsigned i) const {return get(i)->getU32();}
      std::string &getString(unsigned i) {return get(i)->getString();}
      const std::string &getString(unsigned i) const
      {return get(i)->getString();}
      List &getList(unsigned i) {return get(i)->getList();}
      const List &getList(unsigned i) const {return get(i)->getList();}
      Dict &getDict(unsigned i) {return get(i)->getDict();}
      const Dict &getDict(unsigned i) const {return get(i)->getDict();}

      // Dict functions
      virtual const std::string &keyAt(unsigned i) const
      {CBANG_THROW("Not a Dict");}
      virtual int indexOf(const std::string &key) const
      {CBANG_THROW("Not a Dict");}
      bool has(const std::string &key) const {return indexOf(key) != -1;}
      virtual const ValuePtr &get(const std::string &key) const
      {CBANG_THROW("Not a Dict");}
      virtual void insertNull(const std::string &key)
      {CBANG_THROW("Not a Dict");}
      virtual void insertBoolean(const std::string &key, bool value)
      {CBANG_THROW("Not a Dict");}
      virtual void insert(const std::string &key, double value)
      {CBANG_THROW("Not a Dict");}
      virtual void insert(const std::string &key, const std::string &value)
      {CBANG_THROW("Not a Dict");}
      virtual void insert(const std::string &key, const ValuePtr &value)
      {CBANG_THROW("Not a Dict");}

      // Dict accessors
      bool getBoolean(const std::string &key) const
      {return get(key)->getBoolean();}
      double getNumber(const std::string &key) const
      {return get(key)->getNumber();}
      int32_t getS32(const std::string &key) const
      {return get(key)->getS32();}
      uint32_t getU32(const std::string &key) const
      {return get(key)->getU32();}
      std::string &getString(const std::string &key)
      {return get(key)->getString();}
      const std::string &getString(const std::string &key) const
      {return get(key)->getString();}
      List &getList(const std::string &key) {return get(key)->getList();}
      const List &getList(const std::string &key) const
      {return get(key)->getList();}
      Dict &getDict(const std::string &key) {return get(key)->getDict();}
      const Dict &getDict(const std::string &key) const
      {return get(key)->getDict();}

      // Dict accessors with defaults
      bool getBoolean(const std::string &key, bool defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getBoolean();
      }

      double getNumber(const std::string &key, double defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getNumber();
      }

      int32_t getS32(const std::string &key, int32_t defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getS32();
      }

      uint32_t getU32(const std::string &key, uint32_t defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getU32();
      }

      const std::string &getString(const std::string &key,
                                   const std::string &defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getString();
      }

      // Operators
      const Value &operator[](unsigned i) const {return *get(i);}
      const Value &operator[](const std::string &key) const {return *get(key);}

      virtual void write(Sync &sync) const = 0;
      std::string toString() const;
    };

    static inline
    std::ostream &operator<<(std::ostream &stream, const Value &value) {
      Writer writer(stream);
      value.write(writer);
      return stream;
    }
  }
}

#endif // CBANG_JSON_VALUE_H

