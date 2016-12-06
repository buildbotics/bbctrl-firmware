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

#pragma once

#include "ValueRef.h"
#include "Context.h"

#include <cbang/SmartPointer.h>
#include <cbang/js/Impl.h>
#include <cbang/js/Callback.h>

#include <vector>
#include <map>


namespace cb {
  namespace js {class Javascript;}

  namespace gv8 {
    class Module;

    class JSImpl : public js::Impl {
      v8::HandleScope globalScope;
      Context ctx;

      std::vector<SmartPointer<js::Callback> > callbacks;

    public:
      JSImpl(js::Javascript &js);

      static void init(int *argc = 0, char *argv[] = 0);
      static JSImpl &current();

      void add(const SmartPointer<js::Callback> &cb) {callbacks.push_back(cb);}

      // From js::Impl
      SmartPointer<js::Factory> getFactory();
      SmartPointer<js::Scope> enterScope();
      SmartPointer<js::Scope> newScope();
      void interrupt();
    };
  }
}
