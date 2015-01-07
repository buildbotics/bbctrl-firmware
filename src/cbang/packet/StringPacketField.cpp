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

#include "StringPacketField.h"

#include <string.h>

using namespace cb;
using namespace std;


string StringPacketField::toString() const {
  unsigned len;
  for (len = 0; len < length && s[len]; len++) continue;

  return string(s, len);
}


StringPacketField &StringPacketField::operator=(const StringPacketField &pf) {
  *this = pf.toString();
  return *this;
}


const string &StringPacketField::operator=(const string &s) {
  unsigned i;
  for (i = 0; i < length && i < s.length(); i++)
    this->s[i] = s[i];

  for (; i < length; i++) this->s[i] = 0;

  return s;
}
