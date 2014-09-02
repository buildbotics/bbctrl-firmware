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

#include "Version.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <vector>

using namespace std;
using namespace cb;


namespace {
  uint8_t parseVersionPart(const std::string &part) {
    if (part.empty()) THROW("Invalid version string, part is empty");
    if (part.find_first_not_of("0") == string::npos) return 0;
    return String::parseU8(String::trimLeft(part, "0"));
  }
}


Version::Version(const string &s) {
  if (s.find_first_not_of("1234567890. ") != string::npos)
    THROWS("Invalid character in version string: " << s);

  vector<string> parts;
  String::tokenize(s, parts, ".");

  if (parts.empty() || 3 < parts.size())
    THROWS("Error parsing version string: '" << s << "'");

  getMajor() = parseVersionPart(parts[0]);
  if (1 < parts.size()) getMinor() = parseVersionPart(parts[1]);
  if (2 < parts.size()) getRevision() = parseVersionPart(parts[2]);
}


int Version::compare(const Version &v) const {
  if (getMajor() != v.getMajor()) return getMajor() - v.getMajor();
  if (getMinor() != v.getMinor()) return getMinor() - v.getMinor();
  return getRevision() - v.getRevision();
}
