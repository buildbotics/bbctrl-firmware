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

#include "ScriptWebPageHandler.h"

#include "ScriptedWebContext.h"
#include "WebHandler.h"

#include <cbang/script/Handler.h>
#include <cbang/util/SmartToggle.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


bool ScriptWebPageHandler::handlePage(WebContext &ctx, ostream &stream,
                                      const URI &uri) {
  if (evaluating) return false;

  ostringstream str;
  bool handled;

  if (child.isNull()) {
    SmartToggle toggle(evaluating);
    handled = ctx.getWebHandler().handlePage(ctx, str, uri);

  } else handled = child->handlePage(ctx, str, uri);

  if (!handled) return false;

  ctx.setDynamic(); // Don't cache
  Script::Handler *handler = dynamic_cast<ScriptedWebContext *>(&ctx);
  if (!handler) THROWS("Expected Script::Handler");
  Script::Handler::eval(Script::Context(*handler, stream), str.str());

  return true;
}
