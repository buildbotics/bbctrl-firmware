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

#include "WebServer.h"

#include "SessionManager.h"

#include <cbang/log/Logger.h>
#include <cbang/security/ACLSet.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


WebServer::WebServer(Options &options, const string &match,
                     Script::Handler *parent, hasFeature_t hasFeature) :
  Server(options, hasFeature(FEATURE_SSL) ? &sslCtx : 0),
  ScriptedWebHandler(options, match, parent, hasFeature),
  sessionManager(hasFeature(FEATURE_SESSIONS) ?
                 new SessionManager(options) : 0), initialized(false) {}


bool WebServer::_hasFeature(int feature) {
  switch (feature) {
  case FEATURE_SESSIONS: return true;
  case FEATURE_SSL: return false;

  default: return WebHandler::_hasFeature(feature);
  }
}


void WebServer::init() {
  if (initialized) return;
  initialized = true;

  Server::addHandler(this);
  Server::init();
  WebHandler::init();
}


void WebServer::buildResponse(HTTP::Context *_ctx) {
  if (!initialized) THROW("WebServer not initialized");

  WebContext *ctx = dynamic_cast<WebContext *>(_ctx);
  if (!ctx) THROW("Expected WebContext");

  if (hasFeature(FEATURE_SESSIONS)) {
    sessionManager->update();
    sessionManager->findSession(*ctx);
  }

  WebHandler::buildResponse(ctx);
}
