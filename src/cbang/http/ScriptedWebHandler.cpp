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

#include "ScriptedWebHandler.h"

#include "Context.h"
#include "Connection.h"
#include "Response.h"
#include "Request.h"
#include "FileWebPageHandler.h"
#include "ScriptWebPageHandler.h"
#include "ScriptedWebContext.h"

#include <cbang/Info.h>
#include <cbang/SStream.h>
#include <cbang/config/Options.h>
#include <cbang/log/Logger.h>
#include <cbang/xml/XMLWriter.h>
#include <cbang/script/MemberFunctor.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;
using namespace cb::Script;


ScriptedWebHandler::ScriptedWebHandler(Options &options, const string &match,
                       Script::Handler *parent, hasFeature_t hasFeature) :
  WebHandler(options, match, hasFeature), Environment("Web Handler", parent),
  options(options) {

  options.pushCategory("Web Server");
  if (hasFeature(FEATURE_FS_DYNAMIC))
    options.add("web-dynamic", "Path to dynamic web pages.  Empty to disable "
                "filesystem access for dynamic pages.");
  options.popCategory();

  // Dynamic page functions
#define MF_ADD(name, func, min, max)                                    \
  add(new Script::MemberFunctor<ScriptedWebHandler>                     \
      (name, this, &ScriptedWebHandler::func, min, max))

  if (hasFeature(FEATURE_INFO)) MF_ADD("info", evalInfo, 0, 2);
}


void ScriptedWebHandler::init() {
  WebHandler::init();

  if (hasFeature(FEATURE_FS_DYNAMIC) && options["web-dynamic"].hasValue())
    addHandler(new ScriptWebPageHandler
               (new FileWebPageHandler(options["web-dynamic"])));
}


void ScriptedWebHandler::evalInfo(const Script::Context &ctx) {
  Info &info = Info::instance();

  switch (ctx.args.size()) {
  case 1: {
    XMLWriter writer(ctx.stream);
    info.write(writer);
    break;
  }
  case 2: ctx.args.invalidNum(); break;
  case 3: ctx.stream << info.get(ctx.args[1], ctx.args[2]); break;
  }
}


void ScriptedWebHandler::evalOption(const Script::Context &ctx) {
  ctx.stream << options[ctx.args[1]];
}


HTTP::Context *ScriptedWebHandler::createContext(Connection *con) {
  return new ScriptedWebContext(*this, *con);
}
