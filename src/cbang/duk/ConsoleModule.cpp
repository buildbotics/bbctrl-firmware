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


void ConsoleModule::define(Object &exports) {
  // TODO Implement other console.* methods
  // See: https://developer.mozilla.org/en-US/docs/Web/API/Console
  exports.set("log(...)", this, &ConsoleModule::log);
  exports.set("debug(...)", this, &ConsoleModule::debug);
  exports.set("warn(...)", this, &ConsoleModule::warn);
  exports.set("error(...)", this, &ConsoleModule::error);
}


namespace {
  void print(ostream &stream, Arguments &args) {
    for (unsigned i = 0; i < args.size(); i++) {
      if (i) stream << ' ';
      stream << *args.get(i);
    }
  }
}


void ConsoleModule::log(Arguments &args, JSON::Sink &sink) {
  print(*CBANG_LOG_INFO_STREAM(1), args);
}


void ConsoleModule::debug(Arguments &args, JSON::Sink &sink) {
  print(*CBANG_LOG_DEBUG_STREAM(1), args);
}


void ConsoleModule::warn(Arguments &args, JSON::Sink &sink) {
  print(*CBANG_LOG_WARNING_STREAM(), args);
}


void ConsoleModule::error(Arguments &args, JSON::Sink &sink) {
  print(*CBANG_LOG_ERROR_STREAM(), args);
}
