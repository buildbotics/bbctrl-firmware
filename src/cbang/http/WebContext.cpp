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

#include "WebContext.h"

#include "WebContextMethods.h"
#include "WebHandler.h"
#include "Connection.h"
#include "Session.h"

#include <cbang/net/URI.h>
#include <cbang/os/SystemUtilities.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


WebContext::WebContext(WebHandler &handler, Connection &con) :
  Context(con), handler(handler), dynamic(false) {}


WebContext::~WebContext() {} // Hide destructor implementation


const string &WebContext::getUser() const {
  return session->getUser();
}


Script::Environment &WebContext::getEnvironment() {
  if (env.isNull()) env = new Script::Environment("HTTP Context", &handler);
  return *env;
}


bool WebContext::eval(const Script::Context &ctx) {
  // NOTE This dispatch is used instead of mapping the eval functions
  //   directly in to the WebContext to avoid the cost of mapping them
  //   for every HTTP connection.  Could be a bit of a premature optimization
  //   but it's fast.
  WebContextMethods::enableFastParse(); // Enable binary search

  if (getEnvironment().eval(ctx)) return true;

  switch (WebContextMethods::parse(ctx.args[0],
                                   WebContextMethods::WCM_UNKNOWN)) {
  case WebContextMethods::WCM_include:        evalInclude(ctx); break;
  case WebContextMethods::WCM_GET:            evalGet(ctx); break;
  case WebContextMethods::WCM_REMOTE_ADDR:    evalRemoteAddr(ctx); break;
  case WebContextMethods::WCM_REQUEST_URI:    evalRequestURI(ctx); break;
  case WebContextMethods::WCM_REQUEST_METHOD: evalRequestMethod(ctx); break;
  case WebContextMethods::WCM_REQUEST_QUERY:  evalRequestQuery(ctx); break;
  case WebContextMethods::WCM_REQUEST_PATH:   evalRequestPath(ctx); break;
  case WebContextMethods::WCM_UNKNOWN:        return false;
  }

  return true;
}


void WebContext::evalInclude(const Script::Context &ctx) {
  if (ctx.args.size() != 2) ctx.args.invalidNum();

  string path = ctx.handler.eval(ctx.args[1]);

  // TODO move this path code to WebHandler
  if (!path.empty() && path[0] != '/') {
    string parent;
    if (paths.empty()) parent = con.getRequest().getURI().getPath();
    else parent = paths.back();

    // Find last /
    size_t pos = parent.find_last_of('/');
    if (pos != string::npos) path = parent.substr(0, pos + 1) + path;

    // Remove ..'s
    vector<string> parts;
    SystemUtilities::splitPath(path, parts);
    unsigned j = 0;
    for (unsigned i = 0; i < parts.size(); i++) {
      if (parts[i] == "..") {
        if (!j) THROWS("Invalid path: " << path);
        j--;

      } else if (parts[i] != ".") parts[j++] = parts[i];
    }

    parts.resize(j);
    path = SystemUtilities::joinPath(parts);
    if (!parent.empty() && parent[0] == '/') path = string("/") + path;
  }

  paths.push_back(path);
  try {
    handler.handlePage(*this, ctx.stream, path);
  } catch (...) {
    paths.pop_back();
    throw;
  }
  paths.pop_back();
}


void WebContext::evalGet(const Script::Context &ctx) {
  const URI &uri = con.getRequest().getURI();

  if (ctx.args.size() > 1) {
    URI::const_iterator it = uri.find(ctx.args[1]);
    if (it != uri.end()) ctx.stream << it->second;

  } else {
    for (URI::const_iterator it = uri.begin(); it != uri.end(); it++) {
      if (it != uri.begin()) ctx.stream << '&';
      ctx.stream << it->first << '=' << it->second;
    }
  }
}


void WebContext::evalRemoteAddr(const Script::Context &ctx) {
  ctx.stream << con.getClientIP();
}


void WebContext::evalRequestURI(const Script::Context &ctx) {
  ctx.stream << con.getRequest().getURI();
}


void WebContext::evalRequestMethod(const Script::Context &ctx) {
  ctx.stream << con.getRequest().getMethod();
}


void WebContext::evalRequestQuery(const Script::Context &ctx) {
  ctx.stream << con.getRequest().getURI().getQuery();
}


void WebContext::evalRequestPath(const Script::Context &ctx) {
  ctx.stream << con.getRequest().getURI().getPath();
}
