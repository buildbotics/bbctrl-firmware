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

#include "OAuth2Login.h"

#include "SessionManager.h"
#include "WebContext.h"

#include <cbang/http/Connection.h>
#include <cbang/http/Transaction.h>
#include <cbang/json/JSON.h>
#include <cbang/net/Base64.h>
#include <cbang/config/Options.h>
#include <cbang/security/SSLContext.h>
#include <cbang/io/StringInputSource.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


OAuth2Login::OAuth2Login(Options &options,
                         const SmartPointer<SessionManager> &sessionManager,
                         const SmartPointer<SSLContext> &sslCtx) :
  sessionManager(sessionManager), sslCtx(sslCtx) {

  if (sessionManager.isNull())
    this->sessionManager = new SessionManager(options);
  if (sslCtx.isNull()) this->sslCtx = new SSLContext;

  options.pushCategory("OAuth2 Login");
  options.addTarget("oauth2-auth-url", authURL, "OAuth2 Auth URL");
  options.addTarget("oauth2-token-url", tokenURL, "OAuth2 token URL");
  options.addTarget("oauth2-redirect-base", redirectBase,
                    "OAuth2 redirect base URL");
  options.addTarget("oauth2-client-id", clientID, "OAuth2 API Client ID");
  options.addTarget("oauth2-client-secret", clientSecret,
                    "OAuth2 API Client Secret")->setObscured();
  options.popCategory();
}


OAuth2Login::~OAuth2Login() {} // Hide destructor


bool OAuth2Login::handlePage(WebContext &ctx, ostream &stream, const URI &uri) {
  Connection &con = ctx.getConnection();
  Request &request = con.getRequest();
  Response &response = con.getResponse();

  ctx.setDynamic(); // Don't cache

  // Force secure
  if (!con.isSecure())
    THROWCS("Cannot logon via insecure port", StatusCode::HTTP_UNAUTHORIZED);

  // Get session ID
  string sid = request.findCookie(sessionManager->getSessionCookie());
  if (sid.empty() && uri.has("state")) sid = uri.get("state");

  SessionPtr session = sessionManager->findSession(ctx, sid);

  try {
    if (session.isNull() ||
        (uri.has("state") && uri.get("state") != session->getID()) ||
        (!uri.has("state") && session->getUser().empty())) {
      session = sessionManager->openSession(ctx);
      sid = session->getID();

      redirect(ctx, sid);

    } else if (session->getUser().empty()) {
      // TODO Make sure session is not very old

      string email = verify(ctx, session->getID());
      session->setUser(email);
      LOG_INFO(1, "Authorized: " << email);

      // Final redirect to remove auth parameters
      response.redirect(uri.getPath());

    } else return false; // Already authorized

    // Make sure session cookie is set
    sessionManager->setSessionCookie(ctx);

  } catch (...) {
    // Close session on error
    if (!sid.empty()) sessionManager->closeSession(ctx, sid);
    throw;
  }

  return true;
}


void OAuth2Login::redirect(WebContext &ctx, const string &state) {
  Connection &con = ctx.getConnection();
  Request &request = con.getRequest();
  Response &response = con.getResponse();

  // Check config
  if (clientID.empty())
    THROWCS("oauth2-client-id not configured", StatusCode::HTTP_UNAUTHORIZED);
  if (redirectBase.empty())
    THROWCS("oauth2-redirect-base not configured",
            StatusCode::HTTP_UNAUTHORIZED);
  if (authURL.empty())
    THROWCS("oauth2-auth-url not configured", StatusCode::HTTP_UNAUTHORIZED);

  URI uri(authURL);

  uri.set("client_id", clientID);
  uri.set("response_type", "code");
  uri.set("scope", "openid email");
  uri.set("redirect_uri", redirectBase + request.getURI().getPath());
  uri.set("state", state);
  uri.set("max_auth_age", "0");

  response.redirect(uri);
}


string OAuth2Login::verify(WebContext &ctx, const string &state) {
  Request &request = ctx.getConnection().getRequest();
  const URI &uri = request.getURI();

  // Check that SID matches state (Confirm anti-forgery state token)
  if (!uri.has("code") || !uri.has("state") || uri.get("state") != state) {
    LOG_DEBUG(3, "Failed anti-forgery check: uri code="
              << (uri.has("code") ? uri.get("code") : "<null>") << " uri state="
              << (uri.has("state") ? uri.get("state") : "<null>")
              << " server state=" << state);
    THROWCS("Failed anti-forgery check", StatusCode::HTTP_UNAUTHORIZED);
  }

  // Check config
  if (clientID.empty())
    THROWCS("oauth2-client-id not configured", StatusCode::HTTP_UNAUTHORIZED);
  if (clientSecret.empty())
    THROWCS("oauth2-client-secret not configured",
            StatusCode::HTTP_UNAUTHORIZED);
  if (redirectBase.empty())
    THROWCS("oauth2-redirect-base not configured",
            StatusCode::HTTP_UNAUTHORIZED);
  if (tokenURL.empty())
    THROWCS("oauth2-token-url not configured", StatusCode::HTTP_UNAUTHORIZED);

  // Exchange code for access token and ID token
  URI postURI(tokenURL);

  // Setup Query data
  postURI.set("code", uri.get("code"));
  postURI.set("client_id", clientID);
  postURI.set("client_secret", clientSecret);
  postURI.set("redirect_uri", redirectBase + request.getURI().getPath());
  postURI.set("grant_type", "authorization_code");
  string data = postURI.getQuery();
  postURI.setQuery("");

  LOG_DEBUG(5, "Token URI: " << postURI);
  LOG_DEBUG(5, "Token Query: " << data);

  // Verify authorization with OAuth2 server
  Transaction tran(sslCtx.get());
  tran.post(postURI, data.data(), data.length(),
            "application/x-www-form-urlencoded", 1.0);

  // Read response
  tran.receiveHeader();
  stringstream jsonStream;
  transfer(tran, jsonStream);
  string response = jsonStream.str();
  LOG_DEBUG(5, "Token Response Header: " << tran.getResponse());
  LOG_DEBUG(5, "Token Response: " << response);

  // Parse JSON response
  JSON::ValuePtr json = JSON::Reader(StringInputSource(response)).parse();

  // Decode JWT claims
  // See: http://openid.net/specs/draft-jones-json-web-token-07.html#ExampleJWT
  // Note: There is no need to verify the JWT because it comes directly from
  //   the server over a secure connection.
  string idToken = json->getString("id_token");
  vector<string> parts;
  String::tokenize(idToken, parts, ".");
  string claims = Base64(0, '-', '_').decode(parts[1]);
  LOG_DEBUG(5, "Claims: " << claims);
  JSON::ValuePtr claimsJSON =
    JSON::Reader(InputSource(claims.data(), claims.length())).parse();

  // Get & check user email
  string email = claimsJSON->getString("email");
  if (!claimsJSON->getBoolean("email_verified"))
    THROWCS("Email not verified", StatusCode::HTTP_UNAUTHORIZED);

  return email;
}
