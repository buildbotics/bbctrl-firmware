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

#include "BufferEvent.h"
#include "Base.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <event2/bufferevent.h>

#ifdef HAVE_OPENSSL
#include <cbang/security/SSL.h>
#include <cbang/security/SSLContext.h>

#include <event2/bufferevent_ssl.h>

#include <openssl/ssl.h>
#endif // HAVE_OPENSSL

using namespace std;
using namespace cb;
using namespace cb::Event;


BufferEvent::BufferEvent(bufferevent *bev, bool deallocate) :
  bev(bev), deallocate(deallocate) {
}


BufferEvent::BufferEvent(cb::Event::Base &base,
                         const SmartPointer<SSLContext> &sslCtx,
                         const string &host) : bev(0), deallocate(true) {
  if (sslCtx.isNull())
    bev = bufferevent_socket_new(base.getBase(), -1, BEV_OPT_CLOSE_ON_FREE);

#ifdef HAVE_OPENSSL
  else {
    ::SSL *ssl = SSL_new(sslCtx->getCTX());

#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
    if (!host.empty() && host.find_first_not_of("1234567890.") != string::npos)
      SSL_set_tlsext_host_name(ssl, host.c_str());
#endif

    bev = bufferevent_openssl_socket_new
      (base.getBase(), -1, ssl, BUFFEREVENT_SSL_CONNECTING,
       BEV_OPT_DEFER_CALLBACKS);
  }
#else
  else THROW("C! was not built with OpenSSL support");

#endif // HAVE_OPENSSL


  if (!bev) THROW("Failed to create buffer event");

#ifdef HAVE_OPENSSL
  bufferevent_openssl_set_allow_dirty_shutdown(bev, 1);
#endif
}


BufferEvent::~BufferEvent() {
  if (bev && deallocate) bufferevent_free(bev);
}


void BufferEvent::logSSLErrors() {
#ifdef HAVE_OPENSSL
  unsigned error;

  while ((error = bufferevent_get_openssl_error(bev)))
    LOG_ERROR("SSL error: " << SSL::getErrorStr(error));
#endif
}
