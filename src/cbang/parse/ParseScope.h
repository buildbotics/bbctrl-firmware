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

#ifndef CBANG_PARSE_SCOPE_H
#define CBANG_PARSE_SCOPE_H

#include <cbang/LocationRange.h>
#include <cbang/SmartPointer.h>

namespace cb {
  class Scanner;

  class ParseScope {
    cb::Scanner &scanner;
    cb::FileLocation start;

  public:
    ParseScope(Scanner &scanner) :
    scanner(scanner), start(scanner.getLocation()) {}

    void setStart(const cb::FileLocation &start) {this->start = start;}

    void set(cb::LocationRange &location) const
    {location = LocationRange(start, scanner.getLocation());}

    template <typename T>
    SmartPointer<T> set(const SmartPointer<T> &node) const
    {set(node->getLocation()); return node;}
    template <typename T>
    T *set(T *node) const {set(node->getLocation()); return node;}
  };
}

#endif // CBANG_PARSE_SCOPE_H

