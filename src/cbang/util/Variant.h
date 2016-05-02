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

#ifndef CBANG_VARIANT_H
#define CBANG_VARIANT_H

#include <cbang/StdTypes.h>
#include <cbang/String.h>
#include <cbang/SmartPointer.h>
#include <cbang/Exception.h>
#include <cbang/enum/Enumeration.h>

#include <string>
#include <ostream>
#include <typeinfo>


namespace cb {
  class Variant {
  public:
    typedef enum {
      NULL_TYPE,
      BOOLEAN_TYPE,
      STRING_TYPE,
      INTEGER_TYPE,
      UINT64_TYPE,
      REAL_TYPE,
      ENUM_TYPE,
      OBJECT_TYPE,
    } type_t;

    struct Value {
      virtual ~Value() {}

      virtual type_t getType() const = 0;

      virtual bool toBoolean() const {THROW("Cannot convert to boolean");}
      virtual const char *toCString() const
      {THROW("Cannot convert to C string");}
      virtual std::string toString() const {THROW("Cannot convert to string");}
      virtual int64_t toInteger() const {THROW("Cannot convert to integer");}
      virtual uint64_t toUInt64() const {return toInteger();}
      virtual double toReal() const {THROW("Cannot convert to real");}
      virtual void *toObject() const {THROW("Cannot convert to object");}
      virtual const std::type_info &getTypeID() const
      {THROW("Cannot convert to object");}
      virtual SmartPointer<Value> parse(const std::string &value)
      {THROW("Cannot parse from string");}
      virtual const char *getEnumName() const {THROW("Cannot convert to enum");}
      virtual unsigned getEnumCount() const {THROW("Cannot convert to enum");}
      virtual const char *getEnumName(unsigned i) const
      {THROW("Cannot convert to enum");}
      virtual unsigned getEnumValue(unsigned i) const
      {THROW("Cannot convert to enum");}

      virtual int compare(const Value &v) const;
    };


    struct Null : public Value {
      // From Value
      type_t getType() const {return NULL_TYPE;}
      bool toBoolean() const {return false;}
      const char *toCString() const {return "";}
      std::string toString() const {return "";}
      int64_t toInteger() const {return 0;}
      double toReal() const {return 0;}
      SmartPointer<Value> parse(const std::string &value)
      {return SmartPointer<Value>::Phony(this);}
    };


    struct Boolean : public Value {
      bool value;

      Boolean(bool value) : value(value) {}

      // From Value
      type_t getType() const {return BOOLEAN_TYPE;}
      bool toBoolean() const {return value;}
      const char *toCString() const {return value ? "true" : "false";}
      std::string toString() const {return toCString();}
      int64_t toInteger() const {return value;}
      double toReal() const {return value;}
      SmartPointer<Value> parse(const std::string &value)
      {return new Boolean(cb::String::parseBool(value));}
    };


    struct String : public Value {
      std::string value;

      String(const std::string &value) : value(value) {}

      // From Value
      type_t getType() const {return STRING_TYPE;}
      bool toBoolean() const {return cb::String::parseBool(value);}
      const char *toCString() const {return value.c_str();}
      std::string toString() const {return value;}
      int64_t toInteger() const {return cb::String::parseS64(value);}
      double toReal() const {return cb::String::parseDouble(value);}
      SmartPointer<Value> parse(const std::string &value)
      {return new String(value);}
    };


    struct Integer : public Value {
      int64_t value;

      Integer(int64_t value) : value(value) {}

      // From Value
      type_t getType() const {return INTEGER_TYPE;}
      bool toBoolean() const {return value;}
      std::string toString() const {return cb::String(value);}
      int64_t toInteger() const {return value;}
      double toReal() const {return value;}
      SmartPointer<Value> parse(const std::string &value)
      {return new Integer(cb::String::parseS64(value));}
    };


    struct UInt64 : public Value {
      uint64_t value;

      UInt64(uint64_t value) : value(value) {}

      // From Value
      type_t getType() const {return UINT64_TYPE;}
      bool toBoolean() const {return value;}
      std::string toString() const {return cb::String(value);}
      int64_t toInteger() const {return value;}
      uint64_t toUInt64() const {return value;}
      double toReal() const {return value;}
      SmartPointer<Value> parse(const std::string &value)
      {return new Integer(cb::String::parseU64(value));}
    };


    struct Real : public Value {
      double value;

      Real(double value) : value(value) {}

      // From Value
      type_t getType() const {return REAL_TYPE;}
      bool toBoolean() const {return value;}
      std::string toString() const {return cb::String(value);}
      int64_t toInteger() const {return (int64_t)value;}
      double toReal() const {return value;}
      SmartPointer<Value> parse(const std::string &value)
      {return new Real(cb::String::parseDouble(value));}
    };


    template <typename T>
    struct Enum : public Value {
      T value;
      typedef typename T::Enum EnumT; // Enforce Enumeration

      Enum(T value) : value(value) {EnumerationBase &x = value; x = x;}

      // From Value
      type_t getType() const {return ENUM_TYPE;}
      bool toBoolean() const {return value.toInteger();}
      const char *toCString() const {return value.toString();}
      std::string toString() const {return value.toString();}
      int64_t toInteger() const {return value.toInteger();}
      double toReal() const {return value.toInteger();}
      SmartPointer<Value> parse(const std::string &value)
      {return new Enum<T>(T::parse(value));}
      const char *getEnumName() const {return T::getName();}
      unsigned getEnumCount() const {return T::getCount();}
      const char *getEnumName(unsigned i) const {return T::getName(i);}
      unsigned getEnumValue(unsigned i) const {return T::getValue(i);}
    };


    template <typename T>
    struct Object : public Value {
      SmartPointer<T> value;

      Object(const SmartPointer<T> &value) : value(value) {}

      // From Value
      type_t getType() const {return OBJECT_TYPE;}
      bool toBoolean() const {return !value.isNull();}
      std::string toString() const {return value->toString();}
      void *toObject() const {return value.get();}
      SmartPointer<Value> parse(const std::string &value)
      {return new Object(T::parse(value));}
      const std::type_info &getTypeID() const {return typeid(T);}
      int compare(const Value &v) const {return Value::compare(v);}
    };


  protected:
    SmartPointer<Value> value;

    static SmartPointer<Value> null;

  public:
    Variant() : value(null) {}
    explicit Variant(bool value) : value(new Boolean(value)) {}
    Variant(const std::string &value) : value(new String(value)) {}
    Variant(const char *value) : value(new String(value)) {}
    Variant(int8_t value) : value(new Integer((int64_t)value)) {}
    Variant(uint8_t value) : value(new Integer((int64_t)value)) {}
    Variant(int16_t value) : value(new Integer((int64_t)value)) {}
    Variant(uint16_t value) : value(new Integer((int64_t)value)) {}
    Variant(int32_t value) : value(new Integer((int64_t)value)) {}
    Variant(uint32_t value) : value(new Integer((int64_t)value)) {}
    Variant(uint64_t value) : value(new UInt64(value)) {}
    Variant(int64_t value) : value(new Integer(value)) {}
    explicit Variant(double value) : value(new Real(value)) {}
    explicit Variant(float value) : value(new Real(value)) {}
    template <typename T>
    explicit Variant(T value) : value(new Enum<T>(value)) {}
    Variant(const SmartPointer<Value> &value) : value(value) {}

    template <typename T>
    static Variant createObject(const SmartPointer<T> &value)
    {return SmartPointer<Variant::Value>(new Variant::Object<T>(value));}

    type_t getType() const {return value->getType();}

    Variant parse(const std::string &value) const
    {return Variant(this->value->parse(value));}

    void clear() {value = null;}

    void set(bool value) {this->value = new Boolean(value);}
    void set(const std::string &value) {this->value = new String(value);}
    void set(const char *value) {this->value = new String(value);}
    void set(int8_t value) {this->value = new Integer(value);}
    void set(uint8_t value) {this->value = new Integer(value);}
    void set(int16_t value) {this->value = new Integer(value);}
    void set(uint16_t value) {this->value = new Integer(value);}
    void set(int32_t value) {this->value = new Integer(value);}
    void set(uint32_t value) {this->value = new Integer(value);}
    void set(int64_t value) {this->value = new Integer(value);}
    void set(uint64_t value) {this->value = new UInt64(value);}
    void set(double value) {this->value = new Real(value);}
    void set(float value) {this->value = new Real(value);}
    template <typename T>
    void set(T value) {this->value = new Enum<T>(value);}
    template <typename T>
    void set(const SmartPointer<T> &value) {this->value = new Object<T>(value);}

    bool toBoolean() const {return value->toBoolean();}
    const char *toCString() const {return value->toCString();}
    std::string toString() const {return value->toString();}
    int64_t toInteger() const {return value->toInteger();}
    uint64_t toUInt64() const {return value->toUInt64();}
    double toReal() const {return value->toReal();}

    template <typename T>
    T &toObject() const {
      if (!instanceOf<T>()) THROW("Invalid cast");
      return *static_cast<T *>(value->toObject());
    }

    template <typename T>
    bool instanceOf() const {return typeid(T) == value->getTypeID();}

    const char *getEnumName() const {return value->getEnumName();}
    unsigned getEnumCount() const {return value->getEnumCount();}
    const char *getEnumName(unsigned i) const {return value->getEnumName(i);}
    unsigned getEnumValue(unsigned i) const {return value->getEnumValue(i);}

    template <typename T>
    Variant &operator=(T value) {set(value); return *this;}

    int compare(const Variant &v) const {return value->compare(*v.value);}

    bool operator< (const Variant &v) const {return compare(v) <  0;}
    bool operator<=(const Variant &v) const {return compare(v) <= 0;}
    bool operator> (const Variant &v) const {return compare(v) >  0;}
    bool operator>=(const Variant &v) const {return compare(v) >= 0;}
    bool operator==(const Variant &v) const {return compare(v) == 0;}
    bool operator!=(const Variant &v) const {return compare(v) != 0;}
  };


  inline std::ostream &operator<<(std::ostream &stream, const Variant &v) {
    return stream << v.toString();
  }
}

#endif // CBANG_VARIANT_H
