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
#include "ScriptWebPageHandler.h"

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
                       Script::Handler *parent, hasFeature_t hasFeature) :
  HTTP::Handler(match), Environment("Web Handler", parent),
  hasFeature(hasFeature), options(options), initialized(false) {

  options.pushCategory("Web Server");

  if (hasFeature(FEATURE_FS_STATIC))
    options.add("web-static", "Path to static web pages.  Empty to disable "
                "filesystem access for static pages.")->setDefault("www");

  if (hasFeature(FEATURE_FS_DYNAMIC))
    options.add("web-dynamic", "Path to dynamic web pages.  Empty to disable "
                "filesystem access for dynamic pages.");

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

  // Dynamic Page Functions
#define MF_ADD(name, func, min, max)                                    \
  add(new MemberFunctor<WebHandler>(name, this, &WebHandler::func, min, max))

  if (hasFeature(FEATURE_INFO)) MF_ADD("info", evalInfo, 0, 2);
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

  if (hasFeature(FEATURE_FS_DYNAMIC) && options["web-dynamic"].hasValue())
    addHandler(new ScriptWebPageHandler
               (new FileWebPageHandler(options["web-dynamic"])));
}


bool WebHandler::allow(WebContext &ctx) const {
  return ipFilter.isAllowed(ctx.getConnection().getClientIP().getIP());
}


void WebHandler::evalInfo(const Script::Context &ctx) {
  Info &info = Info::instance();

  switch (ctx.args.size()) {
  case 1: {
    XMLWriter writer(ctx.stream);
    info.write(writer);
    break;
  }
  case 2: ctx.args.invalidNum(); break;
  case 3: ctx.stream << info.get(ctx.args[1], ctx.args[2]); break;
  }
}


void WebHandler::evalOption(const Script::Context &ctx) {
  ctx.stream << options[ctx.args[1]];
}


void WebHandler::errorPage(WebContext &ctx, StatusCode status,
                          const string &message) const {
  Connection &con = ctx.getConnection();
  Response &response = con.getResponse();

  response.setStatus(status);
  response.setContentType("text/html");

  string err =
    SSTR((unsigned)status << ' '
         << String::replace(status.toString(), '_', ' ')
         << ' ' << con.getRequest().getURI());

  if (!message.empty()) err += string(": ") + message;

  XMLWriter writer(con);
  writer.simpleElement("h1", err);
  con << flush;

  LOG_WARNING(con << ':' << err);
}


bool WebHandler::handlePage(WebContext &ctx, ostream &stream, const URI &uri) {
  for (unsigned i = 0; i < handlers.size(); i++) {
    if (!handlers[i].second.isNull() &&
        !regex_match(uri.getPath(), *handlers[i].second)) continue;

    if (handlers[i].first->handlePage(ctx, stream, uri)) {

      // Tell client to cache static pages
      if (ctx.isStatic()) ctx.getConnection().getResponse().setCacheExpire();

      // Set default content type
      Response &response = ctx.getConnection().getResponse();
      if (!response.has("Content-Type"))
        response.setContentTypeFromExtension(uri.getPath());

      return true;
    }
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
