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

#ifndef CB_RESTLOGIN_STATE_H
#define CB_RESTLOGIN_STATE_H

#include <cbang/StdTypes.h>
#include <cbang/json/Serializable.h>
#include <cbang/security/KeyPair.h>

#include <string>


namespace cb {
  class RESTLoginState : public JSON::Serializable {
    KeyPair key;

    uint64_t nonce;
    uint64_t ts;
    std::string user;
    uint64_t auth;

  public:
    RESTLoginState(const KeyPair &key);
    RESTLoginState(const KeyPair &key, const std::string &state);
    virtual ~RESTLoginState() {}

    uint64_t getTS() const {return ts;}

    void setUser(const std::string &user) {this->user = user;}
    const std::string &getUser() const {return user;}

    void set(uint64_t auth) {this->auth |= auth;}
    void clear(uint64_t auth) {this->auth &= ~auth;}
    bool allow(uint64_t auth) const {return (this->auth & auth) == auth;}

    virtual std::string toString() const;

    // From JSON::Serializable
    void read(const JSON::Value &value);
    void write(JSON::Sync &sync) const;

  protected:
    std::string encode(const std::string &state) const;
    std::string decode(const std::string &state) const;
  };
}

#endif // CB_RESTLOGIN_STATE_H

