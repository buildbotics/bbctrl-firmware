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

#include "WebHandler.h"

#include "Context.h"
#include "Connection.h"
#include "Response.h"
#include "Request.h"
#include "WebContext.h"
#include "FileWebPageHandler.h"

#include <cbang/Info.h>
#include <cbang/SStream.h>
#include <cbang/config/Options.h>
#include <cbang/log/Logger.h>
#include <cbang/xml/XMLWriter.h>
#include <cbang/script/MemberFunctor.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;
using namespace cb::Script;


WebHandler::WebHandler(Options &options, const string &match,
                       hasFeature_t hasFeature) :
  Features(hasFeature), HTTP::Handler(match), options(options),
  initialized(false) {

  options.pushCategory("Web Server");

  if (hasFeature(FEATURE_FS_STATIC))
    options.add("web-static", "Path to static web pages.  Empty to disable "
                "filesystem access for static pages.")->setDefault("www");

  options.add("web-allow", "Client addresses which are allowed to connect to "
              "this Web server.  This option overrides IPs which are denied in "
              "the web-deny option.  This option differs from the "
              "'allow'/'deny' options in that clients that are not allowed "
              "are served an access denied page rather than simply dropping "
              "the connection.  The value '0/0' matches all IPs."
              )->setDefault("0/0");
  options.add("web-deny", "Client address which are not allowed to connect to "
              "this Web server.")->setDefault("");

  options.popCategory();
}


bool WebHandler::_hasFeature(int feature) {
  return true;
}


void WebHandler::init() {
  if (initialized) return;
  initialized = true;

  // IP filters
  ipFilter.allow(options["web-allow"]);
  ipFilter.deny(options["web-deny"]);

  if (hasFeature(FEATURE_FS_STATIC) && options["web-static"].hasValue())
    addHandler(new FileWebPageHandler(options["web-static"]));
}


bool WebHandler::allow(WebContext &ctx) const {
  return ipFilter.isAllowed(ctx.getConnection().getClientIP().getIP());
}


void WebHandler::errorPage(WebContext &ctx, StatusCode status,
                           const string &message) const {
  ctx.errorPage(status, message);
}


bool WebHandler::handlePage(WebContext &ctx, ostream &stream, const URI &uri) {
  if (WebPageHandlerGroup::handlePage(ctx, stream, uri)) {
    // Tell client to cache static pages
    if (ctx.isStatic()) ctx.getConnection().getResponse().setCacheExpire();

    // Set default content type
    Response &response = ctx.getConnection().getResponse();
    if (!response.has("Content-Type"))
      response.setContentTypeFromExtension(uri.getPath());

    return true;
  }

  return false;
}


HTTP::Context *WebHandler::createContext(Connection *con) {
  return new WebContext(*this, *con);
}


void WebHandler::buildResponse(HTTP::Context *_ctx) {
  if (!initialized) THROW("Not initialized");

  WebContext *ctx = dynamic_cast<WebContext *>(_ctx);
  if (!ctx) THROW("Expected WebContext");

  Connection &con = ctx->getConnection();

  // Check request method
  Request &request = con.getRequest();
  switch (request.getMethod()) {
    case RequestMethod::HTTP_GET:
    case RequestMethod::HTTP_POST:
      break;
  default: return; // We only handle GET and POST
  }

  try {
    if (!allow(*ctx)) errorPage(*ctx, StatusCode::HTTP_UNAUTHORIZED);
    else {
      URI uri = con.getRequest().getURI();
      const string &path = uri.getPath();
      if (path[path.length() - 1] == '/') uri.setPath(path + "index.html");

      // TODO sanitize path

      if (!handlePage(*ctx, con, uri))
        errorPage(*ctx, StatusCode::HTTP_NOT_FOUND);
    }

  } catch (const Exception &e) {
    StatusCode code = StatusCode::HTTP_INTERNAL_SERVER_ERROR;
    if (0 < e.getCode()) code = (StatusCode::enum_t)e.getCode();

    errorPage(*ctx, code, e.getMessage());

    LOG_ERROR(code << ": " << e);
  }

  con << flush;
}
