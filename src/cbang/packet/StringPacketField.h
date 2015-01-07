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

#ifndef CBANG_STRING_PACKET_FIELD_H
#define CBANG_STRING_PACKET_FIELD_H

#include <string>
#include <ostream>

namespace cb {
  class StringPacketField {
    char *s;
    const unsigned length;

  public:
    StringPacketField(void *data, unsigned length) :
      s((char *)data), length(length) {}

    const char *toCString() const {return s;}
    unsigned getLength() const {return length;}
    std::string toString() const;
    StringPacketField &operator=(const StringPacketField &pf);
    const std::string &operator=(const std::string &s);
    operator std::string () const {return toString();}
  };

  inline std::ostream &operator<<(std::ostream &stream,
                                  const StringPacketField &pf) {
    return stream << pf.toString();
  }

  typedef StringPacketField PFStr;
}

#endif // CBANG_STRING_PACKET_FIELD_H

