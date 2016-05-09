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

#ifndef CBANG_PACKET_H
#define CBANG_PACKET_H

#include "PacketField.h"
#include "FPPacketField.h"
#include "StringPacketField.h"
#include "EnumerationPacketField.h"

#include <ostream>
#include <string>

#include <string.h>

namespace cb {
  namespace DB {class Blob;}

  class Packet {
  protected:
    char *data;
    unsigned size;
    bool deallocate;

  public:
    Packet();
    Packet(unsigned size);
    Packet(char *data, unsigned size, bool deallocate = false);
    Packet(const std::string &s);
    Packet(const DB::Blob &blob);
    Packet(const Packet &o);
    Packet(Packet &o, bool steal = false);
    virtual ~Packet();

    Packet &operator=(const Packet &);

    void set(char *data, unsigned size, bool deallocate = false);
    void steal(Packet &packet);
    void copy(Packet &packet);
    void increase(unsigned capacity = 0);

    char *getData() {return data;}
    const char *getData() const {return data;}
    unsigned getSize() const {return size;}

    void clear() {if (data) memset(data, 0, size);}

    virtual std::ostream &print(std::ostream &stream) const {return stream;}
  };

  inline std::ostream &operator<<(std::ostream &stream, const Packet &p) {
    return p.print(stream);
  }
}

#endif // CBANG_PACKET_H
