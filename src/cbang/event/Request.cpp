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

#include "Request.h"
#include "Buffer.h"
#include "Headers.h"

#include <cbang/Exception.h>
#include <cbang/net/URI.h>

#include <event2/http.h>

using namespace cb::Event;
using namespace cb;
using namespace std;


Request::Request(evhttp_request *req) : req(req) {
  if (!req) THROW("Event request cannot be null");
}


Request::~Request() {}


string Request::getHost() const {
  const char *host = evhttp_request_get_host(req);
  return host ? host : "";
}


SmartPointer<URI> Request::getURI() const {
  const char *uri = evhttp_request_get_uri(req);
  return uri ? new URI(uri) : new URI;
}


SmartPointer<Headers> Request::getInputHeaders() const {
  return new Headers(evhttp_request_get_input_headers(req));
}


SmartPointer<Headers> Request::getOutputHeaders() const {
  return new Headers(evhttp_request_get_output_headers(req));
}


void Request::cancel() {
  evhttp_cancel_request(req);
}


void Request::sendError(int error, const string &reason) {
  evhttp_send_error(req, error, reason.c_str());
}


void Request::sendReply(int code, const string &reason, const Buffer &buf) {
  evhttp_send_reply(req, code, reason.c_str(), buf.getBuffer());
}


void Request::sendReplyStart(int code, const string &reason) {
  evhttp_send_reply_start(req, code, reason.c_str());
}


void Request::sendReplyChunk(const Buffer &buf) {
  evhttp_send_reply_chunk(req, buf.getBuffer());
}


void Request::sendReplyEnd() {
  evhttp_send_reply_end(req);
}
