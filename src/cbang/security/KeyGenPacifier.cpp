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

#include "KeyGenPacifier.h"

using namespace std;
using namespace cb;


KeyGenPacifier::KeyGenPacifier(SmartPointer<std::ostream> streamPtr) :
  streamPtr(streamPtr), stream(*streamPtr) {}


KeyGenPacifier::KeyGenPacifier(ostream &stream) : stream(stream) {}


KeyGenPacifier::KeyGenPacifier(const string &prefix, const string &domain,
                               int level) :
  streamPtr(Logger::instance().createStream(domain, level, prefix)),
  stream(*streamPtr) {}


void KeyGenPacifier::operator()(int p) const {
  switch (p) {
  case 0: stream << '.'; break;
  case 1: stream << '+'; break;
  case 2: stream << '*'; break;
  case 3: stream << '-'; break;
  default: stream << '*'; break;
  }

  stream << flush;
}
