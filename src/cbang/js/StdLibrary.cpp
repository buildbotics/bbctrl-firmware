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

#include "StdLibrary.h"
#include "LibraryContext.h"

#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::js;


void StdLibrary::add(ObjectTemplate &tmpl) {
  tmpl.set("print(...)", this, &StdLibrary::print);
  tmpl.set("require(path)", this, &StdLibrary::require);

  // TODO implement other console.* methods
  // See: https://developer.mozilla.org/en-US/docs/Web/API/Console
  console.set("log(...)", this, &StdLibrary::consoleLog);
  console.set("debug(...)", this, &StdLibrary::consoleDebug);
  console.set("warn(...)", this, &StdLibrary::consoleWarn);
  console.set("error(...)", this, &StdLibrary::consoleError);
  tmpl.set("console", console);
}


void StdLibrary::dumpArgs(ostream &stream, const Arguments &args) const {
  for (unsigned i = 0; i < args.getCount(); i++) {
    stream << args[i];
    if (i) stream << ' ';
  }
}


void StdLibrary::print(const Arguments &args) {
  Value JSON = Context::current().getGlobal().get("JSON");
  Value stringify = JSON.get("stringify");

  for (unsigned i = 0; i < args.getCount(); i++)
    if (args[i].isObject())
      ctx.out << stringify.call(JSON, args[i]);
    else ctx.out << args[i];

  ctx.out << flush;
}


Value StdLibrary::require(const Arguments &args) {
  return ctx.require(args.getString("path"));
}


void StdLibrary::consoleLog(const Arguments &args) {
  dumpArgs(*CBANG_LOG_INFO_STREAM(1), args);
}


void StdLibrary::consoleDebug(const Arguments &args) {
  dumpArgs(*CBANG_LOG_DEBUG_STREAM(1), args);
}


void StdLibrary::consoleWarn(const Arguments &args) {
  dumpArgs(*CBANG_LOG_WARNING_STREAM(), args);
}


void StdLibrary::consoleError(const Arguments &args) {
  dumpArgs(*CBANG_LOG_ERROR_STREAM(), args);
}
