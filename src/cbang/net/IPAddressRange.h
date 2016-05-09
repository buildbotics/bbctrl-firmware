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

#ifndef CBANG_IPADDRESS_RANGE_H
#define CBANG_IPADDRESS_RANGE_H

#include "IPAddress.h"

#include <string>
#include <ostream>

namespace cb {
  /**
   * Represents an IP address range.
   *
   * This class parses IP address range strings of the following format:
   *
   *  <domainname>
   *    or
   *  <XXX.XXX.XXX.XXX>[/BB]
   *    or
   *  <XXX.XXX.XXX.XXX>-<XXX.XXX.XXX.XXX>
   *
   * Domain names are resolved with a DSN lookup.
   */
  class IPAddressRange {
    IPAddress start;
    IPAddress end;

  public:
    IPAddressRange(const std::string &spec);
    IPAddressRange(IPAddress ip) : start(ip), end(ip) {}
    IPAddressRange(IPAddress start, IPAddress end);

    std::string toString() const;
    const IPAddress &getStart() const {return start;}
    const IPAddress &getEnd() const {return end;}

    bool contains(const IPAddress &ip) const;
    bool overlaps(const IPAddressRange &range) const;
    bool adjacent(const IPAddressRange &range) const;
    void add(const IPAddressRange &range);

    bool operator<(const IPAddressRange &range) const;

  protected:
    static IPAddress parseIP(const char *&s);
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const IPAddressRange &r) {
    return stream << r.toString();
  }
}

#endif // CBANG_IPADDRESS_RANGE_H
