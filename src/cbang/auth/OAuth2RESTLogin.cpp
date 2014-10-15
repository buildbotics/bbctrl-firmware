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

#include "OAuth2RESTLogin.h"
#include "RESTLoginState.h"
#include "OAuth2.h"

#include <cbang/http/WebContext.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;


OAuth2RESTLogin::OAuth2RESTLogin(const SmartPointer<OAuth2> &auth,
                                 const KeyPair &key) :
  auth(auth), key(key) {}


OAuth2RESTLogin::~OAuth2RESTLogin() {} // Hide destructor


bool OAuth2RESTLogin::handlePage(HTTP::WebContext &ctx, ostream &stream,
                                 const URI &uri) {
  HTTP::Connection &con = ctx.getConnection();
  HTTP::Request &request = con.getRequest();
  HTTP::Response &response = con.getResponse();

  ctx.setDynamic(); // Don't cache

  // Get state
  string state = uri.has("state") ? uri.get("state") : "";

  try {
    if (state.empty()) {
      RESTLoginState state(key);

      URI redirectURL =
        auth->getRedirectURL(uri.getPath(), state.toString());
      response.redirect(redirectURL);

#if 0
    } else if (session->getUser().empty()) {
      // TODO Make sure session is not very old

      JSON::ValuePtr claims = auth->verify(uri, session->getID());
      string email = claims->getString("email");
      if (!claims->getBoolean("email_verified"))
        THROWCS("Email not verified", HTTP::StatusCode::HTTP_UNAUTHORIZED);
      session->setUser(email);
      LOG_INFO(1, "Authorized: " << email);

      // Final redirect to remove auth parameters
      response.redirect(uri.getPath());
#endif

    } else return false; // Already authorized

    // Make sure session cookie is set
    //sessionManager->setRESTCookie(ctx);

  } catch (...) {
    // Close session on error
    //if (!sid.empty()) sessionManager->closeREST(ctx, sid);
    throw;
  }

  return true;
}
