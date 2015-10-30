/******************************************************************************\

             This file is part of the Buildbotics Machine Controller.

                    Copyright (c) 2015-2016, Buildbotics LLC
                               All rights reserved.

        The Buildbotics Machine Controller is free software: you can
        redistribute it and/or modify it under the terms of the GNU
        General Public License as published by the Free Software
        Foundation, either version 2 of the License, or (at your option)
        any later version.

        The Buildbotics Webserver is distributed in the hope that it will
        be useful, but WITHOUT ANY WARRANTY; without even the implied
        warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
        PURPOSE.  See the GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this software.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                        joseph@cauldrondevelopment.com

\******************************************************************************/

#include "Server.h"
#include "App.h"
#include "Transaction.h"

#include <cbang/openssl/SSLContext.h>

#include <cbang/event/Request.h>
#include <cbang/event/PendingRequest.h>
#include <cbang/event/Buffer.h>
#include <cbang/event/RedirectSecure.h>
#include <cbang/event/ResourceHTTPHandler.h>
#include <cbang/event/FileHandler.h>

#include <cbang/config/Options.h>
#include <cbang/util/Resource.h>
#include <cbang/log/Logger.h>

#include <stdlib.h>

using namespace cb;
using namespace std;
using namespace bbmc;

namespace bbmc {
  extern const DirectoryResource resource0;
}


Server::Server(App &app) :
  Event::WebServer(app.getOptions(), app.getEventBase()), app(app) {
}


void Server::init() {
  Event::WebServer::init();

  // Event::HTTP request callbacks
  HTTPHandlerGroup &api = *addGroup(HTTP_ANY, "/api/.*");

#define ADD_TM(GROUP, METHODS, PATTERN, FUNC)                           \
  (GROUP).addMember<Transaction>(METHODS, PATTERN, &Transaction::FUNC)

  // API not found
  ADD_TM(api, HTTP_ANY, "", apiNotFound);

  // Docs
  HTTPHandlerGroup &docs = *addGroup(HTTP_ANY, "/docs/.*");
  if (app.getOptions()["http-root"].hasValue())
    docs.addHandler(app.getOptions()["http-root"]);
  else docs.addHandler(*resource0.find("http"));

  docs.addMember<Transaction>(HTTP_ANY, ".*\\..*", &Transaction::notFound);

  // Root
  if (app.getOptions()["http-root"].hasValue()) {
    string root = app.getOptions()["http-root"];

    addHandler(root);
    addHandler(root + "/index.html");

  } else {
    addHandler(*resource0.find("http"));
    addHandler(*resource0.find("http/index.html"));
  }
}


Event::Request *Server::createRequest(evhttp_request *req) {
  return new Transaction(app, req);
}
