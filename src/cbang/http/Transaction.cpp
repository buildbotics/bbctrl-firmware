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

#include "Transaction.h"

#include "ProxyManager.h"
#include "Request.h"

#include <cbang/String.h>

#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Transaction::~Transaction() {
  try {
    close();
  } CBANG_CATCH_ERROR;
}


void Transaction::send(const Request &request) {
  LOG_DEBUG(5, "Sending (\n" << request << ")");

  request.write(*this);
  flush();
}


void Transaction::receive(Response &response) {
  response.read(*this);
}


void Transaction::get(const URI &uri, float version) {
  if (!isConnected()) connect(IPAddress(uri.getHost(), uri.getPort()));

  Request request(RequestMethod::HTTP_GET, resolve(uri), version);
  request.setPersistent(false);
  ProxyManager::instance().addCredentials(request);

  send(request);
}


void Transaction::post(const URI &uri, const char *data, unsigned length,
                       const string &contentType, float version) {
  if (!isConnected()) connect(IPAddress(uri.getHost(), uri.getPort()));

  Request request(RequestMethod::HTTP_POST, resolve(uri), version);
  request.set("Content-Length", String(length));
  request.set("Content-Type", contentType);
  request.setPersistent(false);
  ProxyManager::instance().addCredentials(request);

  send(request);

  if (data) {write(data, length); flush();}
}


Response &Transaction::exchange(Request &request) {
  ProxyManager::instance().addCredentials(request);
  send(request);

  receiveHeader();

  return response;
}


streamsize Transaction::receiveHeader() {
  // Read HTTP response header
  response.clear();
  receive(response);
  LOG_DEBUG(5, "HTTP response (\n" << response << ")");

  // Intercept proxy authentication requests
  ProxyManager::instance().authenticate(response);

  return response.getContentLength();
}


void Transaction::upload(istream &in, streamsize length,
                         SmartPointer<TransferCallback> callback) {
  transfer(in, *this, length, callback);
}


void Transaction::download(ostream &out, streamsize length,
                           SmartPointer<TransferCallback> callback) {
  transfer(*this, out, length, callback);
}


string Transaction::getString(const URI &uri,
                              SmartPointer<TransferCallback> callback) {
  ostringstream str;
  IPAddress ip(uri.getHost(), uri.getPort());

  connect(ip);
  get(uri);
  download(str, receiveHeader(), callback);

  return str.str();
}


void Transaction::connect(const IPAddress &ip) {
  address = ip;
  if (!address.getPort()) address.setPort(80);

  ProxyManager &proxyMan = ProxyManager::instance();
  if (proxyMan.enabled()) Socket::connect(proxyMan.getAddress());
  else Socket::connect(ip);

  ProxyManager::instance().connect(*this);
}


void Transaction::close() {
  // SocketStream::close(); // This triggers a boost::iostreams bug
  Socket::close();
}


URI Transaction::resolve(const URI &_uri) {
  URI uri = _uri;

  if (uri.getHost().empty()) {
    uri.setHost(address.getHost());
    if (uri.getScheme().empty()) uri.setScheme("http");
    if (!uri.getPort()) uri.setPort(address.getPort());
  }

  return uri;
}
