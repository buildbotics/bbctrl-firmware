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

#ifndef CB_OAUTH2_REST_LOGIN_H
#define CB_OAUTH2_REST_LOGIN_H

#include <cbang/SmartPointer.h>
#include <cbang/http/WebPageHandler.h>
#include <cbang/security/KeyPair.h>

#include <string>


namespace cb {
  class OAuth2;

  namespace HTTP {class WebContext;}

  class OAuth2RESTLogin : public HTTP::WebPageHandler {
    SmartPointer<OAuth2> auth;
    KeyPair key;

  public:
    OAuth2RESTLogin(const SmartPointer<OAuth2> &auth, const KeyPair &key);
    ~OAuth2RESTLogin();

    // From WebPageHandler
    bool handlePage(HTTP::WebContext &ctx, std::ostream &stream,
                    const URI &uri);
  };
}

#endif // CB_OAUTH2_REST_LOGIN_H

