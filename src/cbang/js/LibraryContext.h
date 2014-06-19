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

#ifndef CB_JS_LIBRARY_CONTEXT_H
#define CB_JS_LIBRARY_CONTEXT_H

#include "Value.h"

#include <cbang/SmartPointer.h>

#include <ostream>
#include <map>
#include <vector>
#include <string>


namespace cb {
  namespace js {
    class LibraryContext {
      typedef std::map<std::string, Value> modules_t;
      modules_t modules;

      std::vector<std::string> paths;
      std::vector<std::string> current;

    public:
      std::ostream &out;

      LibraryContext(std::ostream &out) : out(out) {}

      std::vector<std::string> &getPaths() {return paths;}

      Value load(const std::string &path);
    };
  }
}

#endif // CB_JS_LIBRARY_CONTEXT_H

