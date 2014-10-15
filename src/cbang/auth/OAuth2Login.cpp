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
#include "OAuth2.h"

#include <cbang/json/JSON.h>
#include <cbang/net/URI.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;


OAuth2Login::~OAuth2Login() {}


URI OAuth2Login::getRedirectURL(const URI &uri, const string &state) {
  return auth.getRedirectURL(uri.getPath(), state);
}


URI OAuth2Login::getVerifyURL(const URI &uri, const string &state) {
  return auth.getVerifyURL(uri, state);
}


URI OAuth2Login::getProfileURL() {
  return auth.getProfileURL(accessToken);
}


void OAuth2Login::verifyToken(const SmartPointer<JSON::Value> &json) {
  LOG_DEBUG(5, "OAuth2 Token: " << *json);

  // Process claims
  claims = auth.parseClaims(json->getString("id_token"));
  accessToken = json->getString("access_token");

  LOG_INFO(1, "Authenticated: " << claims->getString("email"));
  LOG_DEBUG(3, "Claims: " << *claims);
}


SmartPointer<JSON::Value>
OAuth2Login::processProfile(const SmartPointer<JSON::Value> &profile) {
  return this->profile = auth.processProfile(profile);
}
