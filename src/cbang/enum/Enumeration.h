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

#ifndef CBANG_ENUMERATION_H
#define CBANG_ENUMERATION_H

/***
 * This template is used in combination with 'MakeEnumeration.def' and
 * 'MakeEnumerationImpl.def' to create type-safe enumerations that can be
 * easily converted to and from strings and printed to 'std::ostream'.
 *
 * The method used in the '.def' files is admittedly a bit ugly but it
 * eliminates the need to repeat the enumeration several times.  The
 * '.def' files use what is commonly called X-Macros.  For more information
 * on this technique please see:
 *   http://en.wikipedia.org/wiki/C_preprocessor#X-Macros
 *
 * To create a new enumeration you must create header and implementation files.
 * The header file should look something like this:
 *
 *    #ifndef CBANG_ENUM_EXPAND
 *    #ifndef CBANG_CBANG_ENUM_NAME_H
 *    #define CBANG_CBANG_ENUM_NAME_H
 *
 *    #define CBANG_ENUM_NAME EnumName
 *    #include <cbang/enum/MakeEnumeration.def>
 *
 *    #endif // CBANG_CBANG_ENUM_NAME_H
 *    #else // CBANG_ENUM_EXPAND
 *
 *    CBANG_ENUM_EXPAND(ITEM1, 0) ///< A comment
 *    CBANG_ENUM_EXPAND(ITEM2, 1) ///< A comment
 *    . . .
 *    CBANG_ENUM_EXPAND(ITEMN, N) ///< A comment
 *
 *    #endif // CBANG_ENUM_EXPAND
 *
 * Where 'CBANG_ENUM_NAME' and 'EnumName' are replaced with the name of the
 * enumeration and the 'CBANG_ENUM_EXPAND(name, number)' lines are replaced
 * with the enumeration's names and values.
 *
 * The implementation file is very simple and should look like this:
 *
 *   #define CBANG_ENUM_IMPL
 *   #include "EnumName.h"
 *   #include <cbang/enum/MakeEnumerationImpl.def>
 *
 * Both files should be placed in the directory 'cbang/enum' if they are not
 * then you should also define 'CBANG_ENUM_PATH' in the header before including
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
    using T::toString;
    using T::isValid;
  };

  template <typename T> inline static
  std::ostream &operator<<(std::ostream &stream, const Enumeration<T> &e) {
    return stream << e.toString();
  }
}

#endif // CBANG_ENUMERATION_H

