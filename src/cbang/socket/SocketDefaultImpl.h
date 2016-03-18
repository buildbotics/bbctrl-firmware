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

#ifndef CBANG_SOCKET_DEFAULT_IMPL_H
#define CBANG_SOCKET_DEFAULT_IMPL_H

#include "SocketImpl.h"

#include <cbang/SmartPointer.h>

#include <iostream>

namespace cb {
  /// Create a TCP socket connection.
  class SocketDefaultImpl : public SocketImpl {
    socket_t socket;
    bool blocking;
    bool connected;

    SmartPointer<std::iostream> in;
    SmartPointer<std::iostream> out;

    // Don't allow copy constructor or assignment
    SocketDefaultImpl(const SocketDefaultImpl &o) : SocketImpl(0) {}
    SocketDefaultImpl &operator=(const SocketDefaultImpl &o) {return *this;}

  public:
    SocketDefaultImpl(Socket *parent);

    // From SocketImpl
    bool isOpen() const;
    void setReuseAddr(bool reuse);
    void setBlocking(bool blocking);
    bool getBlocking() const {return blocking;}
    void setKeepAlive(bool keepAlive);
    void setSendBuffer(int size);
    void setReceiveBuffer(int size);
    void setSendTimeout(double timeout);
    void setReceiveTimeout(double timeout);
    void open();
    void bind(const IPAddress &ip);
    void listen(int backlog);
    SmartPointer<Socket> accept(IPAddress *ip);
    void connect(const IPAddress &ip);
    std::streamsize write(const char *data, std::streamsize length,
                          unsigned flags);
    std::streamsize read(char *data, std::streamsize length, unsigned flags);
    void close();
    socket_t get() const {return socket;}

  protected:
    void capture(const IPAddress &addr, bool incoming);
  };
}

#endif // CBANG_SOCKET_DEFAULT_IMPL_H

