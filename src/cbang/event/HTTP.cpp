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

#include "HTTP.h"
#include "Base.h"
#include "Request.h"
#include "RequestCallback.h"

#include <cbang/Exception.h>
#include <cbang/security/SSLContext.h>
#include <cbang/net/IPAddress.h>

#include <event2/http.h>
#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>

using namespace cb::Event;
using namespace std;


namespace {
  struct bufferevent *bev_cb(struct event_base *base, void *arg) {
    return bufferevent_openssl_socket_new(base, -1, SSL_new((SSL_CTX *)arg),
                                          BUFFEREVENT_SSL_ACCEPTING,
                                          BEV_OPT_CLOSE_ON_FREE);
  }


  void request_cb(struct evhttp_request *_req, void *cb) {
    Request req(_req);

    try {
      if (!(*(RequestCallback *)cb)(req))
        req.sendError(404, "Not Found");

    } catch (cb::Exception &e) {
      req.sendError(e.getCode() ? e.getCode() : 500, e.getMessage());

    } catch (std::exception &e) {
      req.sendError(500, e.what());

    } catch (...) {
      req.sendError(500, "Internal Server Error");
    }
  }
}


HTTP::HTTP(const Base &base, const SmartPointer<SSLContext> &sslCtx) :
  http(evhttp_new(base.getBase())), sslCtx(sslCtx) {
  if (!http) THROW("Failed to create event HTTP");

  if (!sslCtx.isNull()) evhttp_set_bevcb(http, bev_cb, sslCtx->getCTX());
}


HTTP::~HTTP() {
  if (http) evhttp_free(http);
}


void HTTP::setMaxBodySize(unsigned size) {
  evhttp_set_max_body_size(http, size);
}


void HTTP::setMaxHeadersSize(unsigned size) {
  evhttp_set_max_headers_size(http, size);
}


void HTTP::setTimeout(int timeout) {
  evhttp_set_timeout(http, timeout);
}


void HTTP::setCallback(const string &path,
                       const SmartPointer<RequestCallback> &cb) {
  int ret = evhttp_set_cb(http, path.c_str(), request_cb, cb.get());
  if (ret)
    THROWS("Failed to set callback on path '" << path << '"'
           << (ret == -1 ? ", Already exists" : ""));
  requestCallbacks.push_back(cb);
}


void HTTP::setGeneralCallback(const SmartPointer<RequestCallback> &cb) {
  evhttp_set_gencb(http, request_cb, cb.get());
  requestCallbacks.push_back(cb);
}


int HTTP::bind(const IPAddress &addr) {
  evhttp_bound_socket *handle =
    evhttp_bind_socket_with_handle(http, addr.getHost().c_str(),
                                   addr.getPort());

  if (!handle) THROWS("Unable to bind HTTP server to " << addr);

  return (int)evhttp_bound_socket_get_fd(handle);
}
