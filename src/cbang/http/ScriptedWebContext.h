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

#ifndef CB_HTTP_SCRIPTED_WEB_CONTEXT_H
#define CB_HTTP_SCRIPTED_WEB_CONTEXT_H

#include "WebContext.h"

#include <cbang/script/Environment.h>

#include <vector>
#include <string>


namespace cb {
  namespace HTTP {
    class ScriptedWebHandler;

    class ScriptedWebContext : public WebContext, public Script::Handler {
      ScriptedWebHandler &handler;
      SmartPointer<Script::Environment> env;
      std::vector<std::string> paths;

    public:
      ScriptedWebContext(ScriptedWebHandler &handler, Connection &con);
      ~ScriptedWebContext();

      Script::Environment &getEnvironment();

      // From Script::Handler
      using Script::Handler::eval;
      bool eval(const Script::Context &ctx);

      void evalInclude(const Script::Context &ctx);
      void evalGet(const Script::Context &ctx);
      void evalRemoteAddr(const Script::Context &ctx);
      void evalRequestURI(const Script::Context &ctx);
      void evalRequestMethod(const Script::Context &ctx);
      void evalRequestQuery(const Script::Context &ctx);
      void evalRequestPath(const Script::Context &ctx);
    };
  }
}

#endif // CB_HTTP_SCRIPTED_WEB_CONTEXT_H
