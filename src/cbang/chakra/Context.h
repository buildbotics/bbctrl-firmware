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

#ifndef CB_CHAKRA_CONTEXT_H
#define CB_CHAKRA_CONTEXT_H

#include "Value.h"

#include <cbang/js/Scope.h>

#include <ChakraCore.h>


namespace cb {
  namespace chakra {
    class JSImpl;

    class Context {
      JSImpl &impl;

      JsContextRef context;

    public:
      class Scope : public js::Scope {
        SmartPointer<Context> ctx;

      public:
        Scope(Context &ctx) :
        ctx(SmartPointer<Context>::Phony(&ctx)) {ctx.enter();}
        Scope(const SmartPointer<Context> &ctx) : ctx(ctx) {ctx->enter();}
        ~Scope() {ctx->leave();}

        // From js::Scope
        SmartPointer<js::Value> getGlobalObject() {
          return new Value(Value::getGlobal());
        }

        SmartPointer<js::Value> eval(const InputSource &source) {
          return new Value(ctx->eval(source));
        }
      };

      Context(JSImpl &impl);

      static Context &current();
      JSImpl &getImpl() {return impl;}

      void enter();
      void leave();

      Value eval(const std::string &path, const std::string &code);
      Value eval(const InputSource &source);
    };
  }
}

#endif // CB_CHAKRA_CONTEXT_H
