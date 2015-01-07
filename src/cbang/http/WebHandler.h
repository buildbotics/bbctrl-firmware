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

#ifndef CB_HTTP_WEB_HANDLER_H
#define CB_HTTP_WEB_HANDLER_H

#include "Handler.h"
#include "WebContext.h"
#include "WebPageHandlerGroup.h"
#include "StatusCode.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/Features.h>
#include <cbang/net/IPAddressFilter.h>

#include <vector>


namespace cb {
  class Options;

  namespace HTTP {
    class WebHandler :
      public Features, public Handler, public WebPageHandlerGroup {
    public:
      enum {
        FEATURE_FS_STATIC,
        FEATURE_LAST,
      };

    protected:
      Options &options;
      IPAddressFilter ipFilter;
      bool initialized;

    public:
      WebHandler(Options &options, const std::string &match = "",
                 hasFeature_t hasFeature = WebHandler::_hasFeature);
      virtual ~WebHandler() {}

      static bool _hasFeature(int feature);

      virtual void init();
      virtual bool allow(WebContext &ctx) const;

      virtual void errorPage(WebContext &ctx, StatusCode status,
                             const std::string &message = std::string()) const;

      // From WebPageHandlerGroup
      bool handlePage(WebContext &ctx, std::ostream &stream,
                      const cb::URI &uri);

      // From Handler
      Context *createContext(Connection *con);
      void buildResponse(Context *ctx);
    };
  }
}

#endif // CB_HTTP_WEB_HANDLER_H

