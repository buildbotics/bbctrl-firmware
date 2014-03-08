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

#ifndef CBANG_IP_RANGE_SET_H
#define CBANG_IP_RANGE_SET_H

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddressRange.h>

#include <string>
#include <vector>
#include <iostream>

namespace cb {namespace JSON {class Sync;}}


namespace cb {
  /// Maintains a set of IP address ranges
  class IPRangeSet {
    typedef std::vector<uint32_t> rangeSet_t;
    rangeSet_t rangeSet;

  public:
    IPRangeSet() {}
    IPRangeSet(const std::string &spec) {insert(spec);}
    ~IPRangeSet() {clear();}

    void clear() {rangeSet.clear();}
    bool empty() {return rangeSet.empty();}

    void insert(const std::string &spec);
    void insert(const IPAddressRange &range);
    void erase(const std::string &spec);
    void erase(const IPAddressRange &range);
    bool contains(const IPAddress &ip) const {return find(ip) & 1;}

    void print(std::ostream &stream) const;
    void write(JSON::Sync &sync) const;

    static SmartPointer<IPRangeSet> parse(const std::string &s)
    {return new IPRangeSet(s);}

  private:
    unsigned find(uint32_t ip) const;
    void shift(int amount, unsigned position);
  };


  static inline
  std::ostream &operator<<(std::ostream &stream, const IPRangeSet &s) {
    s.print(stream);
    return stream;
  }
}

#endif // CBANG_IP_RANGE_SET_H

