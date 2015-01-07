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

#include <cbang/net/IPAddressRange.h>

#include <cctype>

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/log/Logger.h>

#include <algorithm>

using namespace std;
using namespace cb;


IPAddressRange::IPAddressRange(const string &spec) {
  string str = String::trim(spec);

  if (!str.empty()) {
    const char *s = str.c_str();

    if (isalpha(*s)) {
      start = end = IPAddress(str);
      return;

    } else {
      start = parseIP(s);

      if (s[0] == '/' && isdigit(s[1])) {
        string num = string(1, *++s);
        if (isdigit(s[1])) num += *++s;
        uint8_t bits = String::parseU8(num);

        if (32 < bits) THROWS("Invalid IP bit mask /" << bits);
        bits = 32 - bits;

        uint32_t ip = start.getIP();
        uint32_t mask = bits < 32 ? (((uint32_t)1) << bits) - 1 : ~0;

        start = IPAddress(ip & ~mask);
        end = IPAddress((ip & ~mask) | mask);

        return;
      }

      while (isspace(*s)) s++;

      if (*s == 0) {
        end = start;
        return;

      } else if (*s == '-') {
        s++;
        while (isspace(*s)) s++;

        end = parseIP(s);
        if (end < start) swap(start, end);
        if (*s == 0) return;
      }
    }
  }

  THROWS("Invalid IP address range '" << spec << "'");
}


IPAddressRange::IPAddressRange(IPAddress start, IPAddress end) {
  if (start < end) {
    this->start = start;
    this->end = end;

  } else {
    this->start = end;
    this->end = start;
  }
}


string IPAddressRange::toString() const {
  if (start == end) return start.toString();
  return start.toString() + "-" + end.toString();
}


bool IPAddressRange::contains(const IPAddress &ip) const {
  return start.getIP() <= ip.getIP() && ip.getIP() <= end.getIP();
}


bool IPAddressRange::overlaps(const IPAddressRange &range) const {
  return contains(range.start) || contains(range.end) || range.contains(end);
}


bool IPAddressRange::adjacent(const IPAddressRange &range) const {
  return (range.end.getIP() + 1) == start.getIP() ||
    range.start.getIP() == (end.getIP() + 1);
}


void IPAddressRange::add(const IPAddressRange &range) {
  if (!adjacent(range) && !overlaps(range))
    THROW("Ranges are not adjacent and do not overlap");

  if (range.start < start) start = range.start;
  if (end < range.end) end = range.end;
}


bool IPAddressRange::operator<(const IPAddressRange &range) const {
  if (start.getIP() < range.start.getIP()) return true;
  if (range.start.getIP() < start.getIP()) return false;
  if (end.getIP() < range.end.getIP()) return true;
  return false;
}


IPAddress IPAddressRange::parseIP(const char *&s) {
  char buf[16];
  unsigned i = 0;

  while ((isdigit(*s) || *s == '.') && i < 15)
    buf[i++] = *s++;

  buf[i] = 0;

  return IPAddress(buf);
}
