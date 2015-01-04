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

#ifndef CB_JS_CONTEXT_H
#define CB_JS_CONTEXT_H

#include "ObjectTemplate.h"

#include <cbang/SmartPointer.h>

#include "V8.h"


namespace cb {
  namespace js {
    class Context {
      v8::Handle<v8::Context> context;

    public:
      Context(const v8::Handle<v8::Context> &context) : context(context) {}
      Context(ObjectTemplate &tmpl);

      void enter() {context->Enter();}
      void exit() {context->Exit();}

      Value getGlobal() {return v8::Handle<v8::Value>(context->Global());}

      static Context calling() {return v8::Context::GetCalling();}
      static Context current() {return v8::Context::GetCurrent();}
      static Context entered() {return v8::Context::GetEntered();}
      static bool inContext() {return v8::Context::InContext();}
    };
  }
}

#endif // CB_JS_CONTEXT_H

