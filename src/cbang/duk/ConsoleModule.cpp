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

#include "ConsoleModule.h"

#include <cbang/log/Logger.h>

using namespace cb::duk;
using namespace cb;
using namespace std;


void ConsoleModule::define(Object &obj) {
  // TODO Implement other console.* methods
  // See: https://developer.mozilla.org/en-US/docs/Web/API/Console
  obj.set("log(...)", this, &ConsoleModule::log);
  obj.set("debug(...)", this, &ConsoleModule::debug);
  obj.set("warn(...)", this, &ConsoleModule::warn);
  obj.set("error(...)", this, &ConsoleModule::error);
}


Variant ConsoleModule::log(Arguments &args) {
  args.write(*CBANG_LOG_INFO_STREAM(1), " ");
  return Variant();
}


Variant ConsoleModule::debug(Arguments &args) {
  args.write(*CBANG_LOG_DEBUG_STREAM(1), " ");
  return Variant();
}


Variant ConsoleModule::warn(Arguments &args) {
  args.write(*CBANG_LOG_WARNING_STREAM(), " ");
  return Variant();
}


Variant ConsoleModule::error(Arguments &args) {
  args.write(*CBANG_LOG_ERROR_STREAM(), " ");
  return Variant();
}
