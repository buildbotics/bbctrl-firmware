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
#include "HTTPHandler.h"
#include "HTTPStatus.h"
#include "Headers.h"
#include "Buffer.h"

#include <cbang/Exception.h>
#include <cbang/security/SSLContext.h>
#include <cbang/net/IPAddress.h>
#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>

#include <event2/http.h>
#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>

using namespace std;
using namespace cb::Event;


namespace {
  struct bufferevent *bev_cb(struct event_base *base, void *arg) {
    return bufferevent_openssl_socket_new(base, -1, SSL_new((SSL_CTX *)arg),
                                          BUFFEREVENT_SSL_ACCEPTING,
                                          BEV_OPT_CLOSE_ON_FREE);
  }


  void request_cb(struct evhttp_request *_req, void *cb) {
    try {
      Request req(_req);

      req.setIncomming(true);
      req.guessContentType();

      LOG_INFO(1, "< " << req.getMethod() << " " << req.getURI());
      LOG_DEBUG(5, req.getInputHeaders() << '\n');
      LOG_DEBUG(6, req.getInputBuffer().hexdump() << '\n');

      try {
        if (!(*(HTTPHandler *)cb)(req))
          req.sendError(HTTPStatus::HTTP_NOT_FOUND);

      } catch (cb::Exception &e) {
        req.sendError(e.getCode() ? e.getCode() :
                      HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
        if (!CBANG_LOG_DEBUG_ENABLED(3)) LOG_WARNING(e.getMessage());
        LOG_DEBUG(3, e);

      } catch (std::exception &e) {
        req.sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
        LOG_ERROR(e.what());

      } catch (...) {
        req.sendError(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR);
        LOG_ERROR(HTTPStatus(HTTPStatus::HTTP_INTERNAL_SERVER_ERROR)
                  .getDescription());
      }

      LOG_DEBUG(5, req.getResponseLine() << '\n' << req.getOutputHeaders()
                << '\n');
      LOG_DEBUG(6, req.getOutputBuffer().hexdump() << '\n');

    } CATCH_ERROR;
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
                       const SmartPointer<HTTPHandler> &cb) {
  int ret = evhttp_set_cb(http, path.c_str(), request_cb, cb.get());
  if (ret)
    THROWS("Failed to set callback on path '" << path << '"'
           << (ret == -1 ? ", Already exists" : ""));
  requestCallbacks.push_back(cb);
}


void HTTP::setGeneralCallback(const SmartPointer<HTTPHandler> &cb) {
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
