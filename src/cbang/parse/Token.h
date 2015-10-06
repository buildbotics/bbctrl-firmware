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

#ifndef CBANG_TOKEN_H
#define CBANG_TOKEN_H

#include <cbang/LocationRange.h>

#include <string>
#include <ostream>


namespace cb {
  template <class ENUM_T>
  class Token {
    ENUM_T type;
    std::string value;
    cb::LocationRange location;

  public:
    Token(ENUM_T type = eof(),
          const std::string &value = std::string(),
          const cb::LocationRange &location = cb::LocationRange()) :
      type(type), value(value), location(location) {}

    inline static ENUM_T eof() {return (typename ENUM_T::enum_t)0;}

    ENUM_T getType() const {return type;}
    void setType(ENUM_T type) {this->type = type;}

    const std::string &getValue() const {return value;}
    void setValue(const std::string &value) {this->value = value;}

    void set(ENUM_T type, const std::string &value)
    {setType(type); setValue(value);}
    void set(ENUM_T type, char value)
    {setType(type); setValue(std::string(1, value));}

    cb::LocationRange &getLocation() {return location;}
    const cb::LocationRange &getLocation() const {return location;}

    bool operator!() const {return !type;}
    bool operator==(const Token &token) const {return type == token.type;}
    bool operator!=(const Token &token) const {return *this != token;}

    std::ostream &print(std::ostream &stream) const {
      return stream << type << '=' << String::escapeC(value);
    }
  };


  template <class ENUM_T>
  inline static std::ostream &operator<<(std::ostream &stream,
                                         const Token<ENUM_T> &t) {
    return t.print(stream);
  }
}

#endif // CBANG_TOKEN_H

