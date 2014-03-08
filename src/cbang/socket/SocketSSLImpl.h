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

#ifndef CB_SOCKET_SSLIMPL_H
#define CB_SOCKET_SSLIMPL_H

#include "SocketDefaultImpl.h"
#include "BIOSocketImpl.h"

namespace cb {
  class SSL;

  class SocketSSLImpl : public SocketDefaultImpl {
    BIOSocketImpl bio;
    cb::SSL *ssl;

    bool inSSL;

    // Don't allow copy constructor or assignment
    SocketSSLImpl(const SocketSSLImpl &o) :
      SocketDefaultImpl(0, 0), bio(*o.parent) {}
    SocketSSLImpl &operator=(const SocketSSLImpl &o) {return *this;}

  public:
    SocketSSLImpl(Socket *parent, SSLContext *sslCtx);
    ~SocketSSLImpl();

    const cb::SSL *getSSL() const {return ssl;}
    cb::SSL *getSSL() {return ssl;}

    // From SocketImpl
    SmartPointer<Socket> accept(IPAddress *ip);
    void connect(const IPAddress &ip);
    std::streamsize write(const char *data, std::streamsize length,
                          unsigned flags);
    std::streamsize read(char *data, std::streamsize length, unsigned flags);
    void close();
  };
}

#endif // CB_SOCKET_SSLIMPL_H

