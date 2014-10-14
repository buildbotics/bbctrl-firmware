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

#include "Connection.h"
#include "Base.h"
#include "DNSBase.h"
#include "BufferEvent.h"
#include "Request.h"

#include <cbang/Exception.h>
#include <cbang/net/IPAddress.h>
#include <cbang/time/Timer.h>
#include <cbang/security/SSLContext.h>
#include <cbang/log/Logger.h>

#include <event2/http.h>

using namespace cb;
using namespace cb::Event;


namespace {
  evhttp_cmd_type convert_method(unsigned method) {
    switch (method) {
    case RequestMethod::HTTP_GET:     return EVHTTP_REQ_GET;
    case RequestMethod::HTTP_POST:    return EVHTTP_REQ_POST;
    case RequestMethod::HTTP_HEAD:    return EVHTTP_REQ_HEAD;
    case RequestMethod::HTTP_PUT:     return EVHTTP_REQ_PUT;
    case RequestMethod::HTTP_DELETE:  return EVHTTP_REQ_DELETE;
    case RequestMethod::HTTP_OPTIONS: return EVHTTP_REQ_OPTIONS;
    case RequestMethod::HTTP_TRACE:   return EVHTTP_REQ_TRACE;
    case RequestMethod::HTTP_CONNECT: return EVHTTP_REQ_CONNECT;
    case RequestMethod::HTTP_PATCH:   return EVHTTP_REQ_PATCH;
    default: THROWS("Unknown method " << method);
    }
  }
}


Connection::Connection(evhttp_connection *con, bool deallocate) :
  con(con), deallocate(deallocate) {
  if (!con) THROW("Connection cannot be null");
}


Connection::Connection(Base &base, DNSBase &dns, const IPAddress &peer) :
  con(evhttp_connection_base_new(base.getBase(), dns.getDNSBase(),
                                 peer.getHost().c_str(), peer.getPort())),
  deallocate(true) {
  if (!con) THROWS("Failed to create connection to " << peer);
}


Connection::Connection(Base &base, DNSBase &dns, BufferEvent &bev,
                       const IPAddress &peer) :
  con(evhttp_connection_base_bufferevent_new
      (base.getBase(), dns.getDNSBase(), bev.adopt(), peer.getHost().c_str(),
       peer.getPort())), deallocate(true) {
  if (!con) THROWS("Failed to create connection to " << peer);
}


Connection::Connection(Base &base, DNSBase &dns, const URI &uri,
                       const SmartPointer<SSLContext> &sslCtx) :
  con(0), deallocate(true) {
  bool https = uri.getScheme() == "https";

  if (https && sslCtx.isNull()) THROW("Need SSL context for https connection");

  BufferEvent bev(base, https ? sslCtx : 0, uri.getHost());

  con = evhttp_connection_base_bufferevent_new
    (base.getBase(), dns.getDNSBase(), bev.adopt(), uri.getHost().c_str(),
     uri.getPort());

  LOG_DEBUG(5, "Connecting to " << uri.getHost() << ':' << uri.getPort());

  if (!con) THROWS("Failed to create connection to " << uri);
}


Connection::~Connection() {
  if (con && deallocate) evhttp_connection_free(con);
}


IPAddress Connection::getPeer() const {
  IPAddress peer;

  char *addr;
  ev_uint16_t port;
  evhttp_connection_get_peer(con, &addr, &port);
  if (addr) peer = IPAddress(addr, port);

  const sockaddr *sa = evhttp_connection_get_addr(con);
  if (sa && sa->sa_family == AF_INET) {
    peer.setPort(((sockaddr_in *)sa)->sin_port);
    peer.setIP(((sockaddr_in *)sa)->sin_addr.s_addr);
  }

  return peer;
}


void Connection::setMaxBodySize(unsigned size) {
  evhttp_connection_set_max_body_size(con, size);
}


void Connection::setMaxHeaderSize(unsigned size) {
  evhttp_connection_set_max_headers_size(con, size);
}


void Connection::setInitialRetryDelay(double delay) {
  struct timeval tv = Timer::toTimeVal(delay);
  evhttp_connection_set_initial_retry_tv(con, &tv);
}


void Connection::setRetries(unsigned retries) {
  evhttp_connection_set_retries(con, retries);
}


void Connection::setTimeout(double timeout) {
  struct timeval tv = Timer::toTimeVal(timeout);
  evhttp_connection_set_timeout_tv(con, &tv);
}


void Connection::makeRequest(Request &req, unsigned method, const URI &uri) {
  evhttp_cmd_type m = convert_method(method);

  if (evhttp_make_request(con, req.adopt(), m, uri.toString().c_str()))
    THROWS("Failed to make request to " << uri);
}


void Connection::logSSLErrors() {
  bufferevent *bev = evhttp_connection_get_bufferevent(con);
  if (bev) BufferEvent(bev, false).logSSLErrors();
}
