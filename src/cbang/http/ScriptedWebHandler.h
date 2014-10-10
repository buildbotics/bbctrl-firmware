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

#ifndef CB_HTTP_SCRIPTED_WEB_HANDLER_H
#define CB_HTTP_SCRIPTED_WEB_HANDLER_H

#include "WebHandler.h"

#include <cbang/script/Environment.h>


namespace cb {
  class Options;

  namespace HTTP {
    class ScriptedWebHandler : public WebHandler, public Script::Environment {
    public:
      enum {
        FEATURE_FS_DYNAMIC = WebHandler::FEATURE_LAST,
        FEATURE_INFO,
        FEATURE_LAST,
      };

    protected:
      Options &options;
      bool initialized;

    public:
      ScriptedWebHandler(Options &options, const std::string &match = "",
                 Script::Handler *parent = 0,
                 hasFeature_t hasFeature = ScriptedWebHandler::_hasFeature);
      virtual ~ScriptedWebHandler() {}

      static bool _hasFeature(int feature);

      virtual void init();

      void evalInfo(const Script::Context &ctx);
      void evalOption(const Script::Context &ctx);

      // From Handler
      Context *createContext(Connection *con);
    };
  }
}

#endif // CB_HTTP_SCRIPTED_WEB_HANDLER_H

