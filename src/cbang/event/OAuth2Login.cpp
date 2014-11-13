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
#include "Request.h"
#include "PendingRequest.h"
#include "Client.h"

#include <cbang/auth/OAuth2.h>
#include <cbang/net/URI.h>
#include <cbang/json/Value.h>
#include <cbang/util/DefaultCatch.h>

using namespace std;
using namespace cb::Event;


OAuth2Login::OAuth2Login(Client &client) : client(client), auth(0) {}


OAuth2Login::~OAuth2Login() {}


bool OAuth2Login::authorize(Request &req, cb::OAuth2 &auth,
                            const string &state) {
  if (req.getURI().has("state")) return requestToken(req, auth, state);
  return authRedirect(req, auth, state);
}


bool OAuth2Login::authRedirect(Request &req, cb::OAuth2 &auth,
                               const string &state) {
  req.redirect(auth.getRedirectURL(req.getURI().getPath(), state));
  return true;
}


bool OAuth2Login::requestToken(Request &req, cb::OAuth2 &auth,
                               const string &state) {
  this->auth = &auth;
  this->state = state;

  URI verifyURL = auth.getVerifyURL(req.getURI(), state);

  // Extract query data
  string data = verifyURL.getQuery();
  verifyURL.setQuery("");

  // Verify authorization with OAuth2 server
  pending = client.callMember
    (verifyURL, HTTP_POST, data.data(), data.length(), this,
     &OAuth2Login::verifyToken);
  pending->setContentType("application/x-www-form-urlencoded");
  pending->outSet("Accept", "application/json");
  pending->send();

  return true;
}


bool OAuth2Login::verifyToken(Request &req) {
  try {
    if (!auth) THROW("Auth is null");

    // Verify
    string accessToken = auth->verifyToken(req.getInput());

    // Get profile
    pending = client.callMember(auth->getProfileURL(accessToken), HTTP_GET,
                                this, &OAuth2Login::processProfile);
    pending->outSet("User-Agent", "cbang.org");
    pending->send();

    return true;
  } CATCH_ERROR;

  processProfile(0); // Notify incase of error

  return true;
}


bool OAuth2Login::processProfile(Request &req) {
  SmartPointer<JSON::Value> profile;

  try {
    if (!auth) THROW("Auth is null");
    profile = req.getInputJSON();
    if (profile.isNull()) THROW("Did not receive profile");
    profile = auth->processProfile(profile);
  } CATCH_ERROR;

  processProfile(profile);

  // Cleanup
  auth = 0;
  pending.release();
  state.clear();

  return true;
}
