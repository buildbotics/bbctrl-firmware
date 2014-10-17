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

#include "GitHubOAuth2.h"

#include <cbang/String.h>
#include <cbang/net/URI.h>
#include <cbang/config/Options.h>
#include <cbang/json/JSON.h>

using namespace cb;
using namespace std;


GitHubOAuth2::GitHubOAuth2(Options &options) : OAuth2("github") {
  authURL = "https://github.com/login/oauth/authorize";
  tokenURL = "https://github.com/login/oauth/access_token";
  profileURL = "https://api.github.com/user";
  scope = "user:email";

  options.pushCategory("GitHub OAuth2 Login");
  OAuth2::addOptions(options);
  options.popCategory();
}


SmartPointer<JSON::Value>
GitHubOAuth2::processProfile(const SmartPointer<JSON::Value> &profile) const {
  SmartPointer<JSON::Value> p = new JSON::Dict;

  p->insert("provider", provider);
  p->insert("id", String(profile->getNumber("id")));
  p->insert("name", profile->getString("name"));
  p->insert("email", profile->getString("email"));
  p->insert("avatar", profile->getString("avatar_url"));
  p->insertBoolean("verified", true);
  p->insert("raw", profile);

  return p;
}
