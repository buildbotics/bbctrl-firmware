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

#include "GoogleOAuth2.h"

#include <cbang/net/URI.h>
#include <cbang/config/Options.h>
#include <cbang/json/JSON.h>

using namespace cb;
using namespace std;


GoogleOAuth2::GoogleOAuth2(Options &options, const string &maxAuthAge) :
  OAuth2("google"), maxAuthAge(maxAuthAge) {
  authURL = "https://accounts.google.com/o/oauth2/auth";
  tokenURL = "https://accounts.google.com/o/oauth2/token";
  profileURL = "https://www.googleapis.com/plus/v1/people/me/openIdConnect";
  scope = "openid email profile";

  options.pushCategory("Google OAuth2 Login");
  OAuth2::addOptions(options);
  options.popCategory();
}


URI GoogleOAuth2::getRedirectURL(const string &path,
                                 const string &state) const {
  URI url = OAuth2::getRedirectURL(path, state);
  if (!maxAuthAge.empty()) url.set("max_auth_age", maxAuthAge);
  return url;
}


SmartPointer<JSON::Value>
GoogleOAuth2::processProfile(const SmartPointer<JSON::Value> &profile) const {
  SmartPointer<JSON::Value> p = new JSON::Dict;

  p->insert("provider", provider);
  p->insert("id", profile->getString("sub"));
  p->insert("name", profile->getString("name"));
  p->insert("email", profile->getString("email"));
  p->insert("avatar", profile->getString("picture"));
  p->insertBoolean("verified", profile->getBoolean("email_verified"));
  p->insert("raw", profile);

  return p;
}
