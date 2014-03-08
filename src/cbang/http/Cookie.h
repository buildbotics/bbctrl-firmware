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

#ifndef CB_HTTP_COOKIE_H
#define CB_HTTP_COOKIE_H

#include <cbang/StdTypes.h>

#include <string>


namespace cb {
  namespace HTTP {
    class Cookie {
      std::string name;
      std::string value;
      std::string domain;
      std::string path;
      uint64_t expires;
      uint64_t maxAge;
      bool httpOnly;
      bool secure;

    public:
      Cookie(const std::string &name = std::string(),
             const std::string &value = std::string(),
             const std::string &domain = std::string(),
             const std::string &path = std::string(),
             uint64_t expires = 0, uint64_t maxAge = 0,
             bool httpOnly = false, bool secure = false) :
        name(name), value(value), domain(domain), path(path), expires(expires),
        maxAge(maxAge), httpOnly(httpOnly), secure(secure) {}

      const std::string &getName() const {return name;}
      void setName(const std::string &name) {this->name = name;}

      const std::string &getValue() const {return value;}
      void setValue(const std::string &value) {this->value = value;}

      const std::string &getDomain() const {return domain;}
      void setDomain(const std::string &domain) {this->domain = domain;}

      const std::string &getPath() const {return path;}
      void setPath(const std::string &path) {this->path = path;}

      uint64_t getExpires() const {return expires;}
      void setExpires(uint64_t &expires) {this->expires = expires;}

      uint64_t getMaxAge() const {return maxAge;}
      void setMaxAge(uint64_t &maxAge) {this->maxAge = maxAge;}

      bool getHttpOnly() const {return httpOnly;}
      void setHttpOnly(bool &httpOnly) {this->httpOnly = httpOnly;}

      bool getSecure() const {return secure;}
      void setSecure(bool &secure) {this->secure = secure;}

      std::string toString() const;
      void read(const std::string &s);
    };
  }
}

#endif // CB_HTTP_COOKIE_H

