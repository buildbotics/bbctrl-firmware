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

#include "PendingRequest.h"
#include "Client.h"
#include "Buffer.h"
#include "BufferEvent.h"

#include <cbang/String.h>
#include <cbang/security/SSLContext.h>

using namespace cb;
using namespace cb::Event;


PendingRequest::PendingRequest(Client &client, const URI &uri, unsigned method,
                               const SmartPointer<HTTPHandler> &cb) :
  Connection(client.getBase(), client.getDNS(), uri, client.getSSLContext()),
  Request(cb), uri(uri), method(method) {
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
}
