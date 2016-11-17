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

#include "Javascript.h"

#ifdef HAVE_CHAKRA
#include <cbang/chakra/JSImpl.h>
#endif

#include <cbang/util/SmartFunctor.h>

using namespace cb::js;
using namespace cb;
using namespace std;


Javascript::Javascript() : impl(0), stdMod(*this) {
#ifdef HAVE_CHAKRA
  impl = new chakra::JSImpl(*this);
#else
  THROW("No Javscript implementation compiled in");
#endif

  define(stdMod);
  define(consoleMod);

  import("std", ".");
  import("console");
}


void Javascript::define(Module &mod) {impl->define(mod);}


void Javascript::import(const string &module, const string &as) {
  impl->import(module, as);
}


void Javascript::exec(const InputSource &source) {
  pushPath(source.getName());
  SmartFunctor<Javascript> popPath(this, &Javascript::popPath);
  impl->exec(source);
}


void Javascript::interrupt() {impl->interrupt();}
