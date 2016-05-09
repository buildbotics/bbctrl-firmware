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

#ifndef CBANG_JSON_STRING_H
#define CBANG_JSON_STRING_H

#include "Value.h"

#include <cbang/String.h>

#include <string>

namespace cb {
  namespace JSON {
    class String : public Value {
      std::string s;

    public:
      String(const std::string &s = std::string()) : s(s) {}

      void setValue(const std::string &s) {this->s = s;}
      std::string &getValue() {return s;}
      const std::string &getValue() const {return s;}

      operator const std::string &() const {return s;}

      // From Value
      ValueType getType() const {return JSON_STRING;}
      ValuePtr copy(bool deep = false) const {return new String(s);}
      bool getBoolean() const {return cb::String::parseBool(getString());}
      double getNumber() const {return cb::String::parseDouble(getString());}
      std::string &getString() {return getValue();}
      const std::string &getString() const {return getValue();}
      void write(Sink &sink) const {sink.write(s);}
    };
  }
}

#endif // CBANG_JSON_STRING_H
