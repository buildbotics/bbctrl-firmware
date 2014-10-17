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

#ifndef CB_OAUTH2_H
#define CB_OAUTH2_H

#include <cbang/SmartPointer.h>

#include <string>


namespace cb {
  class URI;
  class Options;
  namespace JSON {class Value;}

  class OAuth2 {
  protected:
    std::string provider;

    std::string authURL;
    std::string tokenURL;
    std::string profileURL;
    std::string redirectBase;
    std::string clientID;
    std::string clientSecret;
    std::string scope;

  public:
    OAuth2(const std::string &prefix);
    virtual ~OAuth2();

    const std::string &getProvider() const {return provider;}

    virtual URI getRedirectURL(const std::string &path,
                               const std::string &state) const;
    virtual URI getVerifyURL(const URI &uri, const std::string &state) const;
    virtual URI getProfileURL(const std::string &token) const;

    virtual std::string verifyToken(const std::string &data) const;

    virtual SmartPointer<JSON::Value>
    processProfile(const SmartPointer<JSON::Value> &profile) const = 0;

  protected:
    void addOptions(Options &options);
    void validateOption(const std::string &option,
                        const std::string &name) const;
  };
}

#endif // CB_OAUTH2_H

