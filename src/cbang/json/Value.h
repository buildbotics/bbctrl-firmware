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
    class Sink;

    typedef SmartPointer<Value> ValuePtr;

    class Value : public ValueType::Enum {
    public:
      virtual ~Value() {}

      virtual ValueType getType() const = 0;
      virtual ValuePtr copy(bool deep = false) const = 0;
      virtual bool canWrite(Sink &sink) const {return true;}

      virtual bool isSimple() const {return true;}
      bool isUndefined() const {return getType() == JSON_UNDEFINED;}
      bool isNull() const {return getType() == JSON_NULL;}
      bool isBoolean() const {return getType() == JSON_BOOLEAN;}
      bool isNumber() const {return getType() == JSON_NUMBER;}
      bool isString() const {return getType() == JSON_STRING;}
      bool isList() const {return getType() == JSON_LIST;}
      bool isDict() const {return getType() == JSON_DICT;}

      template <typename T> bool is() {return dynamic_cast<T *>(this);}

      template <typename T> T &cast() {
        T *ptr = dynamic_cast<T *>(this);
        if (!ptr) THROW("Invalid cast");
        return *ptr;
      }

      template <typename T> const T &cast() const {
        const T *ptr = dynamic_cast<T *>(this);
        if (!ptr) THROW("Invalid cast");
        return *ptr;
      }

      virtual bool getBoolean() const {CBANG_THROW("Value is not a Boolean");}
      virtual double getNumber() const {CBANG_THROW("Value is not a Number");}
      int32_t getS32() const;
      uint32_t getU32() const;
      int64_t getS64() const;
      uint64_t getU64() const;
      virtual std::string &getString() {CBANG_THROW("Value is not a String");}
      virtual const std::string &getString() const
      {CBANG_THROW("Value is not a String");}
      virtual List &getList() {CBANG_THROW("Value is not a List");}
      virtual const List &getList() const {CBANG_THROW("Value is not a List");}
      virtual Dict &getDict() {CBANG_THROW("Value is not a Dict");}
      virtual const Dict &getDict() const {CBANG_THROW("Value is not a Dict");}

      virtual void setBoolean(bool value)
      {CBANG_THROW("Value is not a Boolean");}
      virtual void set(double value) {CBANG_THROW("Value is not a Number");}
      void set(int8_t value) {set((double)value);}
      void set(uint8_t value) {set((double)value);}
      void set(int16_t value) {set((double)value);}
      void set(uint16_t value) {set((double)value);}
      void set(int32_t value) {set((double)value);}
      void set(uint32_t value) {set((double)value);}
      void set(int64_t value) {set((double)value);}
      void set(uint64_t value) {set((double)value);}
      virtual void set(const std::string &value) {CBANG_THROW("Not a String");}

      virtual unsigned size() const {CBANG_THROW("Not a List or Dict");}

      // List functions
      virtual const ValuePtr &get(unsigned i) const
      {CBANG_THROW("Not a List or Dict");}
      void appendUndefined();
      void appendNull();
      void appendBoolean(bool value);
      void append(double value);
      void append(float value) {append((double)value);}
      void append(int8_t value) {append((double)value);}
      void append(uint8_t value) {append((double)value);}
      void append(int16_t value) {append((double)value);}
      void append(uint16_t value) {append((double)value);}
      void append(int32_t value) {append((double)value);}
      void append(uint32_t value) {append((double)value);}
      void append(int64_t value) {append(String(value));}
      void append(uint64_t value) {append(String(value));}
      void append(const std::string &value);
      virtual void append(const ValuePtr &value) {CBANG_THROW("Not a List");}

      // List accessors
      bool getBoolean(unsigned i) const {return get(i)->getBoolean();}
      double getNumber(unsigned i) const {return get(i)->getNumber();}
      int32_t getS32(unsigned i) const {return get(i)->getS32();}
      uint32_t getU32(unsigned i) const {return get(i)->getU32();}
      int64_t getS64(unsigned i) const {return get(i)->getS64();}
      uint64_t getU64(unsigned i) const {return get(i)->getU64();}
      std::string &getString(unsigned i) {return get(i)->getString();}
      const std::string &getString(unsigned i) const
      {return get(i)->getString();}
      List &getList(unsigned i) {return get(i)->getList();}
      const List &getList(unsigned i) const {return get(i)->getList();}
      Dict &getDict(unsigned i) {return get(i)->getDict();}
      const Dict &getDict(unsigned i) const {return get(i)->getDict();}

      // List setters
      virtual void set(unsigned i, const ValuePtr &value)
      {CBANG_THROW("Not a List");}
      void setUndefined(unsigned i);
      void setNull(unsigned i);
      void setBoolean(unsigned i, bool value);
      void set(unsigned i, double value);
      void set(unsigned i, int8_t value) {set(i, (double)value);}
      void set(unsigned i, uint8_t value) {set(i, (double)value);}
      void set(unsigned i, int16_t value) {set(i, (double)value);}
      void set(unsigned i, uint16_t value) {set(i, (double)value);}
      void set(unsigned i, int32_t value) {set(i, (double)value);}
      void set(unsigned i, uint32_t value) {set(i, (double)value);}
      void set(unsigned i, int64_t value) {set(i, (double)value);}
      void set(unsigned i, uint64_t value) {set(i, (double)value);}
      void set(unsigned i, const std::string &value);

      // Dict functions
      virtual const std::string &keyAt(unsigned i) const
      {CBANG_THROW("Not a Dict");}
      virtual int indexOf(const std::string &key) const
      {CBANG_THROW("Not a Dict");}
      bool has(const std::string &key) const {return indexOf(key) != -1;}
      bool hasNull(const std::string &key) const {
        int index = indexOf(key);
        return index == -1 ? false : get(index)->isNull();
      }
      bool hasBoolean(const std::string &key) const {
        int index = indexOf(key);
        return index == -1 ? false : get(index)->isBoolean();
      }
      bool hasNumber(const std::string &key) const {
        int index = indexOf(key);
        return index == -1 ? false : get(index)->isNumber();
      }
      bool hasString(const std::string &key) const {
        int index = indexOf(key);
        return index == -1 ? false : get(index)->isString();
      }
      bool hasList(const std::string &key) const {
        int index = indexOf(key);
        return index == -1 ? false : get(index)->isList();
      }
      bool hasDict(const std::string &key) const {
        int index = indexOf(key);
        return index == -1 ? false : get(index)->isDict();
      }
      virtual const ValuePtr &get(const std::string &key) const
      {CBANG_THROW("Not a Dict");}
      void insertUndefined(const std::string &key);
      void insertNull(const std::string &key);
      void insertBoolean(const std::string &key, bool value);
      void insert(const std::string &key, double value);
      void insert(const std::string &key, float value)
      {insert(key, (double)value);}
      void insert(const std::string &key, uint8_t value)
      {insert(key, (double)value);}
      void insert(const std::string &key, int8_t value)
      {insert(key, (double)value);}
      void insert(const std::string &key, uint16_t value)
      {insert(key, (double)value);}
      void insert(const std::string &key, int16_t value)
      {insert(key, (double)value);}
      void insert(const std::string &key, uint32_t value)
      {insert(key, (double)value);}
      void insert(const std::string &key, int32_t value)
      {insert(key, (double)value);}
      void insert(const std::string &key, uint64_t value)
      {insert(key, String(value));}
      void insert(const std::string &key, int64_t value)
      {insert(key, String(value));}
      void insert(const std::string &key, const std::string &value);
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
      int64_t getS64(const std::string &key) const
      {return get(key)->getS64();}
      uint64_t getU64(const std::string &key) const
      {return get(key)->getU64();}
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

      int64_t getS64(const std::string &key, int64_t defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getS64();
      }

      uint64_t getU64(const std::string &key, uint64_t defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getU64();
      }

      const std::string &getString(const std::string &key,
                                   const std::string &defaultValue) const {
        int index = indexOf(key);
        return index == -1 ? defaultValue : get(index)->getString();
      }

      // Operators
      const Value &operator[](unsigned i) const {return *get(i);}
      const Value &operator[](const std::string &key) const {return *get(key);}

      virtual void write(Sink &sink) const = 0;
      std::string toString(unsigned indent = 0, bool compact = false) const;
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
