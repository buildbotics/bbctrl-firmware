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

#ifndef CB_HTTP_OAUTH2_LOGIN_H
#define CB_HTTP_OAUTH2_LOGIN_H

#include "WebPageHandler.h"

#include <cbang/SmartPointer.h>

#include <string>


namespace cb {
  class Options;
  class SSLContext;

  namespace HTTP {
    class WebContext;
    class SessionManager;

    class OAuth2Login : public WebPageHandler {
      SmartPointer<SessionManager> sessionManager;
      SmartPointer<SSLContext> sslCtx;

      std::string authURL;
      std::string tokenURL;
      std::string redirectBase;
      std::string clientID;
      std::string clientSecret;

    public:
      OAuth2Login(Options &options,
                  const SmartPointer<SessionManager> &sessionManager = 0,
                  const SmartPointer<SSLContext> &sslCtx = 0);
      ~OAuth2Login();

      // From WebPageHandler
      bool handlePage(WebContext &ctx, std::ostream &stream,
                      const cb::URI &uri);

    protected:
      void redirect(WebContext &ctx, const std::string &state);
      std::string verify(WebContext &ctx, const std::string &state);
    };
  }
}

#endif // CB_HTTP_OAUTH2_LOGIN_H

