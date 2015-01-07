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

#ifndef CBANG_PYON_MESSAGE_H
#define CBANG_PYON_MESSAGE_H

#include "Header.h"

#include <cbang/json/Value.h>

namespace cb {
  namespace PyON {
    class Message : public Header {
      JSON::ValuePtr value;

    public:
      Message(const std::string &type, const JSON::ValuePtr &value,
              const char *magic = CBANG_PYON_MAGIC,
              uint32_t version = CBANG_PYON_VERSION) :
        Header(type, magic, version), value(value) {}
      Message(const char *magic = CBANG_PYON_MAGIC,
              uint32_t version = CBANG_PYON_VERSION) :
        Header(std::string(), magic, version) {}

      JSON::ValuePtr get() const {return value;}

      void write(std::ostream &stream) const;
      void read(std::istream &stream);
    };


    static inline
    std::ostream &operator<<(std::ostream &stream, const Message &m) {
      m.write(stream); return stream;
    }

    static inline
    std::istream &operator>>(std::istream &stream, Message &m) {
      m.read(stream); return stream;
    }
  }
}

#endif // CBANG_PYON_MESSAGE_H

