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

#ifndef CBANG_URI_H
#define CBANG_URI_H

#include <cbang/util/StringMap.h>

#include <string>

namespace cb {
  class URI : public StringMap {
  protected:
    std::string scheme;
    std::string host;
    unsigned port;
    std::string path;
    std::string user;
    std::string pass;

  public:
    URI();
    URI(const std::string &uri);
    URI(const char *uri);

    const std::string &getScheme() const {return scheme;}
    const std::string &getHost() const {return host;}
    unsigned getPort() const;
    const std::string &getPath() const {return path;}
    const std::string &getUser() const {return user;}
    const std::string &getPass() const {return pass;}
    const std::string getQuery() const;

    void setScheme(const std::string &scheme) {this->scheme = scheme;}
    void setHost(const std::string &host) {this->host = host;}
    void setPort(unsigned port) {this->port = port;}
    void setPath(const std::string &path) {this->path = path;}
    void setUser(const std::string &user) {this->user = user;}
    void setPass(const std::string &pass) {this->pass = pass;}
    void setQuery(const std::string &query);

    void normalize();

    void read(const std::string &data);
    std::ostream &write(std::ostream &stream) const;
    std::ostream &writeQuery(std::ostream &stream) const;

    std::string toString() const;
    operator std::string() const {return toString();}

    static int hex2digit(char x);
    static std::string encode(const std::string &s);
    static std::string decode(const std::string &s);
  };

  inline std::ostream &operator<<(std::ostream &stream, const URI &uri) {
    return uri.write(stream);
  }
}

#endif // CBANG_URI_H

