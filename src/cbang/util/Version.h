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

#ifndef CB_VERSION_H
#define CB_VERSION_H

#include <cbang/StdTypes.h>
#include <cbang/SStream.h>
#include <cbang/geom/Vector.h>

#include <ostream>

namespace cb {
  // TODO Remove the dependency on Vector
  class Version : public Vector<3, uint8_t> {
  public:
    Version(uint8_t verMajor = 0, uint8_t verMinor = 0,
            uint8_t verRevision = 0) :
      Vector<3, uint8_t>(verMajor, verMinor, verRevision) {}

    Version(uint32_t v):
      Vector<3, uint8_t>((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff) {}

    Version(const std::string &s);

    uint8_t getMajor() const {return data[0];}
    uint8_t getMinor() const {return data[1];}
    uint8_t getRevision() const {return data[2];}
    uint8_t &getMajor() {return data[0];}
    uint8_t &getMinor() {return data[1];}
    uint8_t &getRevision() {return data[2];}

    int compare(const Version &v) const;

    bool operator<(const Version &v) const {return compare(v) < 0;}
    bool operator<=(const Version &v) const {return compare(v) <= 0;}
    bool operator>(const Version &v) const {return compare(v) > 0;}
    bool operator>=(const Version &v) const {return compare(v) >= 0;}
    bool operator==(const Version &v) const {return compare(v) == 0;}
    bool operator!=(const Version &v) const {return compare(v) != 0;}

    operator uint32_t () const {
      return ((uint32_t)getMajor() << 16) | ((uint32_t)getMinor() << 8) |
        getRevision();
    }

    operator std::string () const {return toString();}

    std::string toString() const {
      return CBANG_SSTR((int)getMajor() << '.' << (int)getMinor() << '.'
                        << (int)getRevision());
    }
  };


  inline static
  std::ostream &operator<<(std::ostream &stream, const Version &v) {
    return stream << v.toString();
  }
}

#endif // CB_VERSION_H

