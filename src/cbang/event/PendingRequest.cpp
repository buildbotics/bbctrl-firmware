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

#include "PendingRequest.h"
#include "Client.h"
#include "Buffer.h"
#include "Headers.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>
#include <cbang/debug/Debugger.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/os/SysError.h>

#include <event2/http.h>

using namespace cb;
using namespace cb::Event;


namespace {
  void request_cb(struct evhttp_request *req, void *pr) {
    ((PendingRequest *)pr)->callback(req);
  }


  void error_cb(enum evhttp_request_error error, void *pr) {
    ((PendingRequest *)pr)->error(error);
  }
}


PendingRequest::PendingRequest(Client &client, const URI &uri, unsigned method,
                               const SmartPointer<HTTPHandler> &cb) :
  Connection(client.getBase(), client.getDNS(), uri, client.getSSLContext()),
  Request(evhttp_request_new(request_cb, this), uri, true), uri(uri),
  method(method), cb(cb) {
  evhttp_request_set_error_cb(req, error_cb);
}


void PendingRequest::send() {
  // Set output headers
  if (!outHas("Host")) outSet("Host", uri.getHost());
  if (!outHas("Connection")) outSet("Connection", "close");

  // Set Content-Length
  switch (method) {
  case HTTP_POST:
  case HTTP_PUT:
  case HTTP_PATCH:
    if (!outHas("Content-Length"))
      outSet("Content-Length", String(getOutputBuffer().getLength()));
    break;
  default: break;
  }

  // Do it
  makeRequest(*this, method, uri);

  LOG_INFO(1, "> " << getMethod() << " " << getURI());
  LOG_DEBUG(5, getOutputHeaders() << '\n');
  LOG_DEBUG(6, getOutputBuffer().hexdump() << '\n');
}


void PendingRequest::callback(evhttp_request *_req) {
  try {
    if (!_req) return;
    Request req(_req);

    LOG_DEBUG(5, req.getResponseLine() << '\n' << req.getInputHeaders()
              << '\n');
    LOG_DEBUG(6, req.getInputBuffer().hexdump() << '\n');

    (*cb)(req);
  } CATCH_ERROR;
}


void PendingRequest::error(int code) {
  LOG_ERROR("Request failed: " << getErrorStr(code)
            << ", System error: " << SysError()
            << ", SSL errors: " << getSSLErrors());

  LOG_DEBUG(5, Debugger::getStackTrace());

  try {cb->error(code);} CATCH_ERROR;
}
