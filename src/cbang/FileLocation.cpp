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

#include "FileLocation.h"

#include <iostream>

using namespace std;
using namespace cb;

bool FileLocation::operator==(const FileLocation &o) const {
  return filename == o.filename && line == o.line && col == o.col &&
    function == o.function;
}


ostream &cb::operator<<(ostream &stream, const FileLocation &fl) {
  if (!fl.isEmpty()) {
    stream << fl.filename;
    if (fl.line >= 0) {
      stream << ':' << fl.line;

      if (fl.col >= 0) stream << ':' << fl.col;
    }

    if (fl.function != "") stream << ':' << fl.function << "()";
  }

  return stream;
}

