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

#include "JSONAPI.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>
#include <cbang/json/JSON.h>
#include <cbang/json/List.h>
#include <cbang/http/WebContext.h>
#include <cbang/http/Connection.h>
#include <cbang/http/StatusCode.h>


using namespace cb;
using namespace cb::JSAPI;
using namespace std;


void JSONAPI::add(const string &cmd, const SmartPointer<Handler> &handler) {
  if (!api.insert(api_t::value_type(cmd, handler)).second)
    THROWS("'" << root << cmd << "' already exists in JSON API");
}


bool JSONAPI::handlePage(HTTP::WebContext &ctx, ostream &stream,
                         const URI &uri) {
  if (!String::startsWith(uri.getPath(), root)) return false;

  string cmd = uri.getPath().substr(root.length());

  // Look up command
  api_t::const_iterator it = api.find(cmd);
  if (it == api.end()) return false;

  ctx.setDynamic(); // Don't cache
  HTTP::Connection &con = ctx.getConnection();

  bool jsonp = !jsonpCB.empty() && uri.has(jsonpCB);

  if (jsonp) {
    con.getResponse().setContentType("application/javascript");
    con << uri.get(jsonpCB) << '(';

  } else con.getResponse().setContentType("application/json");

  JSON::Writer writer(con, 0, true);

  try {
    // Parse JSON data
    JSON::ValuePtr msg;
    if (con.getRequest().hasContentType() &&
        String::startsWith(con.getRequest().getContentType(),
                           "application/json")) {
      MemoryBuffer &payload = con.getPayload();
      if (payload.getFill()) msg = JSON::Reader(payload).parse();

    } else if (!uri.empty()) {
      msg = new JSON::Dict;

      for (URI::const_iterator it = uri.begin(); it != uri.end(); it++)
        msg->insert(it->first, it->second);
    }

    // Dispatch API command
    if (msg.isNull()) LOG_DEBUG(5, "JSONAPI Call: " << cmd << "()");
    else LOG_DEBUG(5, "JSONAPI Call: " << cmd << '(' << *msg << ')');
    it->second->handle(ctx, cmd, msg, writer);

    // Make sure JSON stream is complete
    writer.close();

  } catch (const Exception &e) {
    LOG_ERROR(e);

    // Clear possibly partial or invalid response
    con.clearResponseBuffer();

    // jsonp header
    if (jsonp) con << uri.get(jsonpCB) << '(';

    // Send error message
    JSON::Writer writer(con, 0, true);
    writer.beginList();
    writer.append("error");
    writer.append(e.getMessage());
    writer.endList();
    writer.close();
  }

  if (jsonp) con << ");";

  return true;
}
