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
#ifdef HAVE_OPENSSL

#include "SocketSSLImpl.h"

#include "Socket.h"

#include <cbang/Zap.h>
#include <cbang/Exception.h>

#include <cbang/security/SSL.h>
#include <cbang/security/SSLContext.h>

#include <cbang/util/SmartToggle.h>

using namespace cb;
using namespace std;


SocketSSLImpl::SocketSSLImpl(Socket *parent, SSLContext *sslCtx) :
  SocketDefaultImpl(parent, sslCtx), bio(*parent),
  ssl(sslCtx->createSSL(bio.getBIO())), inSSL(false) {}


SocketSSLImpl::~SocketSSLImpl() {
  zap(ssl);
}


SmartPointer<Socket> SocketSSLImpl::accept(IPAddress *ip) {
  SmartPointer<Socket> socket = SocketDefaultImpl::accept(ip);
  if (socket.isNull()) return 0;

  SocketSSLImpl *impl = dynamic_cast<SocketSSLImpl *>(socket->getImpl());
  if (impl) {
    SmartToggle toggle(impl->inSSL);
    impl->ssl->accept();

  } else THROW("Expected SSL socket implementation");

  return socket;
}


void SocketSSLImpl::connect(const IPAddress &ip) {
  SocketDefaultImpl::connect(ip);
  SmartToggle toggle(inSSL);
  ssl->connect();
}


streamsize SocketSSLImpl::write(const char *data, streamsize length,
                                unsigned flags) {
  if (!length) return 0;

  if (inSSL) return SocketDefaultImpl::write(data, length, flags);
  else {
    SmartToggle toggle(inSSL);
    streamsize ret = ssl->write(data, length);

    if (SSL::peekError()) THROWS("SSL read error " << SSL::getErrorStr());

    if (!bio.getException().isNull()) throw *bio.getException();
    return ret;
  }
}


streamsize SocketSSLImpl::read(char *data, streamsize length, unsigned flags) {
  if (!length) return 0;

  if (inSSL) return SocketDefaultImpl::read(data, length, flags);
  else {
    SmartToggle toggle(inSSL);
    streamsize ret = ssl->read(data, length);

    if (SSL::peekError()) THROWS("SSL read error " << SSL::getErrorStr());

    if (!bio.getException().isNull()) throw *bio.getException();
    return ret;
  }
}


void SocketSSLImpl::close() {
  if (isOpen()) {
    SmartToggle toggle(inSSL);
    ssl->shutdown();
  }
  SocketDefaultImpl::close();
}

#endif // HAVE_OPENSSL
