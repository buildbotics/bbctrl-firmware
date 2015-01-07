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

#ifndef CBANG_ENUMERATION_H
#define CBANG_ENUMERATION_H

/***
 * This template is used in combination with 'MakeEnumeration.def' and
 * 'MakeEnumerationImpl.def' to create type-safe enumerations that can be
 * easily converted to and from strings and printed to 'std::ostream'.
 *
 * The method used in the '.def' files is admittedly a bit ugly but it
 * eliminates the need to repeat the enumeration several times and the
 * uglyness is confined to two files.  Repeated code is more evil than ugly
 * code.  The '.def' files use what is commonly called X-Macros.  For more
 * information on this technique please see:
 *
 *   http://en.wikipedia.org/wiki/C_preprocessor#X-Macros
 *
 * To create a new enumeration you must create header and implementation files.
 * The header file should look something like this:
 *
 *    #ifndef CBANG_ENUM
 *    #ifndef CBANG_ENUM_NAME_H
 *    #define CBANG_ENUM_NAME_H
 *
 *    #define CBANG_ENUM_NAME EnumName
 *    #include <cbang/enum/MakeEnumeration.def>
 *
 *    #endif // CBANG_ENUM_NAME_H
 *    #else // CBANG_ENUM
 *
 *    CBANG_ENUM(ITEM1)
 *    CBANG_ENUM(ITEM2)
 *    . . .
 *    CBANG_ENUM(ITEMN)
 *
 *    #endif // CBANG_ENUM
 *
 * Where 'CBANG_ENUM_NAME' and 'EnumName' are replaced with the name of the
 * enumeration and the 'CBANG_ENUM(name)' lines are replaced with the
 * enumeration's names.
 *
 * Three other macros are also defined with allow you to specify more
 * information about the enumeration's entries.  These are:
 *
 *   CBANG_ENUM_VALUE(name, value)
 *   CBANG_ENUM_DESC(name, "description")
 *   CBANG_ENUM_VALUE_DESC(name, value, "description")
 *
 * These macros can be used in place of CBANG_ENUM for some or all of the
 * entries.  If value is not specified it defaults to 0 for the first
 * entry and the previous value + 1 for each proceeding entry.  If description
 * is not specified it defaults to the enumeration entry's name.
 *
 * The implementation file is very simple and should look like this:
 *
 *   #define CBANG_ENUM_IMPL
 *   #include "EnumName.h"
 *   #include <cbang/enum/MakeEnumerationImpl.def>
 *
 * Both files should be placed in the directory 'cbang/enum' if they are not
 * then you must define 'CBANG_ENUM_PATH' in the header before including
 * 'cbang/enum/MakeEnumeration.def'.
 *
 * To use the enumeration use the class EnumName defined in the header file.
 * The enumeration file names should match the 'EnumName'.
 *
 * 'EnumName' will be an instantiation of the 'Enumeration' template defined
 * below.  The actual definition looks like this:
 *
 *   typedef Enumeration<EnumNameEnumeration> EnumName;
 *
 * 'EnumNameEnumeration' will be defined by the X-Macros.
 */

#include <cbang/String.h>
#include <cbang/StdTypes.h>

#include <string>
#include <ostream>

namespace cb {
  class EnumerationBase {
  protected:
    uint32_t value;

  public:
    EnumerationBase(uint32_t x) : value(x) {}

    uint32_t &getIntegerRef() {return value;}
    uint32_t toInteger() const {return value;}
  };


  template <typename T>
  class Enumeration : public EnumerationBase, public T {
  public:
    typedef typename T::enum_t enum_t;

    Enumeration() : EnumerationBase(0) {}
    Enumeration(enum_t value) : EnumerationBase((unsigned)value) {}

    operator enum_t() const {return (enum_t)value;}

    const char *getDescription() const {return getDescription((enum_t)value);}
    const char *toString() const {return toString((enum_t)value);}
    bool isValid() const {return isValid((enum_t)value);}
    static std::string getFlagsString(uint32_t flags) {
      std::string s;

      bool first = true;
      for (unsigned i = 0; i < T::getCount(); i++)
        if ((flags & T::getValue(i)) == (uint32_t)T::getValue(i)) {
          if (!first) s += " | ";
          else first = false;
          s += T::getName(i);
        }

      return s;
    }

    using T::parse;
    using T::getDescription;
    using T::toString;
    using T::isValid;
  };


  template <typename T> inline static
  std::ostream &operator<<(std::ostream &stream, const Enumeration<T> &e) {
    return stream << e.toString();
  }
}

#endif // CBANG_ENUMERATION_H

