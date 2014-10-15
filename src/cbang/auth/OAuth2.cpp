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

#include "OAuth2.h"

#include <cbang/Exception.h>
#include <cbang/json/JSON.h>
#include <cbang/net/Base64.h>
#include <cbang/net/URI.h>
#include <cbang/config/Options.h>
#include <cbang/io/StringInputSource.h>
#include <cbang/log/Logger.h>
#include <cbang/http/StatusCode.h>

using namespace std;
using namespace cb;


OAuth2::OAuth2() {}
OAuth2::~OAuth2() {} // Hide destructor


URI OAuth2::getRedirectURL(const string &path, const string &state) const {
  // Check config
  if (clientID.empty())
    THROWCS(prefix + "client-id not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);
  if (redirectBase.empty())
    THROWCS(prefix + "redirect-base not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);
  if (authURL.empty())
    THROWCS(prefix + "auth-url not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);
  if (scope.empty())
    THROWCS(prefix + "scope not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);

  // Build redirect URL
  URI uri(authURL);
  uri.set("client_id", clientID);
  uri.set("response_type", "code");
  uri.set("scope", scope);
  uri.set("redirect_uri", redirectBase + path);
  uri.set("state", state);

  return uri;
}


URI OAuth2::getVerifyURL(const URI &uri, const string &state) const {
  // Check that SID matches state (Confirm anti-forgery state token)
  if (!uri.has("code") || !uri.has("state") || uri.get("state") != state) {
    LOG_DEBUG(3, "Failed anti-forgery check: uri code="
              << (uri.has("code") ? uri.get("code") : "<null>") << " uri state="
              << (uri.has("state") ? uri.get("state") : "<null>")
              << " server state=" << state);
    THROWCS("Failed anti-forgery check", HTTP::StatusCode::HTTP_UNAUTHORIZED);
  }

  // Check config
  if (clientID.empty())
    THROWCS(prefix + "client-id not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);
  if (clientSecret.empty())
    THROWCS(prefix + "client-secret not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);
  if (redirectBase.empty())
    THROWCS(prefix + "redirect-base not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);
  if (tokenURL.empty())
    THROWCS(prefix + "token-url not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);

  // Exchange code for access token and ID token
  URI postURI(tokenURL);

  // Setup Query data
  postURI.set("code", uri.get("code"));
  postURI.set("client_id", clientID);
  postURI.set("client_secret", clientSecret);
  postURI.set("redirect_uri", redirectBase + uri.getPath());
  postURI.set("grant_type", "authorization_code");

  return postURI;
}


URI OAuth2::getProfileURL(const string &accessToken) const {
  if (profileURL.empty())
    THROWCS(prefix + "profile-url not configured",
            HTTP::StatusCode::HTTP_UNAUTHORIZED);

  URI url(profileURL);
  url.set("access_token", accessToken);
  return url;
}


SmartPointer<JSON::Value>
OAuth2::processProfile(const SmartPointer<JSON::Value> &profile) const {
  return profile;
}


SmartPointer<JSON::Value> OAuth2::parseClaims(const std::string &token) const {
  // Decode JWT claims
  // See: http://openid.net/specs/draft-jones-json-web-token-07.html#ExampleJWT
  // Note: There is no need to verify the JWT because it comes directly from
  //   the server over a secure connection.
  vector<string> parts;
  String::tokenize(token, parts, ".");

  string claims = Base64(0, '-', '_').decode(parts[1]);
  LOG_DEBUG(5, "Claims: " << claims);

  return JSON::Reader(StringInputSource(claims)).parse();
}


void OAuth2::addOptions(Options &options, const string &prefix) {
  this->prefix = prefix;
  options.addTarget(prefix + "auth-url", authURL, "OAuth2 auth URL");
  options.addTarget(prefix + "token-url", tokenURL, "OAuth2 token URL");
  options.addTarget(prefix + "redirect-base", redirectBase,
                    "OAuth2 redirect base URL");
  options.addTarget(prefix + "client-id", clientID, "OAuth2 API client ID");
  options.addTarget(prefix + "client-secret", clientSecret,
                    "OAuth2 API client secret")->setObscured();
  options.addTarget(prefix + "scope", scope, "OAuth2 API scope");
}
