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

#ifndef CB_CHAKRA_JSIMPL_H
#define CB_CHAKRA_JSIMPL_H

#include "ValueRef.h"
#include "Context.h"

#include <cbang/SmartPointer.h>
#include <cbang/js/Impl.h>
#include <cbang/js/Callback.h>

#include <Jsrt/ChakraCore.h>

#include <vector>
#include <map>


namespace cb {
  namespace js {class Javascript;}

  namespace chakra {
    class Module;

    class JSImpl : public js::Impl {
      js::Javascript &js;

      JsRuntimeHandle runtime;
      SmartPointer<Context> ctx;
      SmartPointer<ValueRef> common;

      std::vector<SmartPointer<js::Callback> > callbacks;

      typedef std::map<std::string, SmartPointer<Module> > modules_t;
      modules_t modules;

    public:
      JSImpl(js::Javascript &js);
      ~JSImpl();

      const JsRuntimeHandle &getRuntime() const {return runtime;}
      static JSImpl &current();

      void add(const SmartPointer<js::Callback> &cb) {callbacks.push_back(cb);}
      Value require(const std::string &id);
      void enable();

      // From js::Impl
      void define(js::Module &mod);
      void import(const std::string &module, const std::string &as);
      void exec(const InputSource &source);
      void interrupt();
    };
  }
}

#endif // CB_CHAKRA_JSIMPL_H
