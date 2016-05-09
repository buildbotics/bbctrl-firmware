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

#ifndef CBANG_PYON_OBJECT_H
#define CBANG_PYON_OBJECT_H

#include <cbang/SmartPointer.h>
#include <cbang/json/Value.h>

#include <iostream>


namespace cb {
  namespace PyON {
    class Object {
    public:
      virtual ~Object() {}

      virtual const char *getPyONType() const = 0;
      virtual SmartPointer<JSON::Value> getJSON() const = 0;
      virtual void loadJSON(const JSON::Value &value) = 0;

      virtual void write(std::ostream &stream) const;
      virtual void read(std::istream &stream);
    };


    static inline
    std::ostream &operator<<(std::ostream &stream, const Object &obj) {
      obj.write(stream); return stream;
    }

    static inline
    std::istream &operator>>(std::istream &stream, Object &obj) {
      obj.read(stream); return stream;
    }
  }
}

#endif // CBANG_PYON_OBJECT_H
