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

#ifndef CB_CHAKRA_JAVASCRIPT_H
#define CB_CHAKRA_JAVASCRIPT_H

#include "PathResolver.h"
#include "ConsoleModule.h"
#include "StdModule.h"
#include "Impl.h"

#include <cbang/io/InputSource.h>


namespace cb {
  namespace js {
    class Javascript : public PathResolver {
      SmartPointer<Impl> impl;

      StdModule stdMod;
      ConsoleModule consoleMod;

      typedef std::map<std::string, SmartPointer<Module> > modules_t;
      modules_t modules;

      SmartPointer<Value> nativeProps;

    public:
      Javascript(const std::string &implName = std::string());

      SmartPointer<js::Factory> getFactory();
      void define(NativeModule &mod);
      void import(const std::string &module,
                  const std::string &as = std::string());
      SmartPointer<js::Value> eval(const InputSource &source);
      void interrupt();

      // Callbacks
      SmartPointer<Value> require(Callback &cb, Value &args);
    };
  }
}

#endif // CB_CHAKRA_JAVASCRIPT_H
