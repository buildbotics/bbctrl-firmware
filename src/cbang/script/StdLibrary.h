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

#ifndef CBANG_SCRIPT_STD_LIBRARY_H
#define CBANG_SCRIPT_STD_LIBRARY_H

#include <cbang/util/Singleton.h>

#include "Environment.h"

namespace cb {
  namespace Script {
    class Context;

    class StdLibrary : public Singleton<StdLibrary>, public Environment {
      StdLibrary();
      ~StdLibrary() {}

    public:
      StdLibrary(Inaccessible);

      static void evalIf(const Context &ctx);
      static void evalNot(const Context &ctx);
      static void evalEval(const Context &ctx);
      static void evalClear(const Context &ctx);
      static void evalDate(const Context &ctx);
      static void evalSleep(const Context &ctx);
      static void evalAdd(const Context &ctx);
      static void evalSub(const Context &ctx);
      static void evalMul(const Context &ctx);
      static void evalDiv(const Context &ctx);
      static void evalEq(const Context &ctx);
      static void evalNeq(const Context &ctx);
      static void evalLess(const Context &ctx);
    };
  }
}

#endif // CBANG_SCRIPT_STD_LIBRARY_H
