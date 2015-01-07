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

#include "Request.h"
#include "ProxyManager.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Request::Request(RequestMethod method, const URI &uri, float version) :
  Message(version), method(method), uri(uri) {
  if (version != 1.0) {
    set("Host", uri.getHost());
    if ((uri.getScheme() == "http" && uri.getPort() != 80) ||
        (uri.getScheme() == "https" && uri.getPort() != 443))
      append("Host", String(uri.getPort()));
  }
}


string Request::getHeaderLine() const {
  string requestURI;
  if (version == 1.0 || ProxyManager::instance().enabled())
    requestURI = uri.toString();
  else requestURI = uri.getPath();

  return
    string(method.toString()) + " " + requestURI + " " + getVersionString();
}


void Request::readHeaderLine(const string &line) {
  vector<string> parts;
  String::tokenize(line, parts);

  if (parts.size() != 3) THROWS("Invalid HTTP request '" << line << "'");

  setMethod(RequestMethod::parse(parts[0]));
  uri.read(parts[1]);
  readVersionString(parts[2]);
}

