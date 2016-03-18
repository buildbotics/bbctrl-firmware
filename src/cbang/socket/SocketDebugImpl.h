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

#ifndef CBANG_SOCKET_DEBUG_IMPL_H
#define CBANG_SOCKET_DEBUG_IMPL_H

#include "SocketImpl.h"

#include <cbang/SmartPointer.h>

namespace cb {
  class SocketDebugConnection;

  class SocketDebugImpl : public SocketImpl {
    bool socketOpen;
    bool blocking;
    bool listening;
    bool bound;

    IPAddress ip;

    SocketDebugConnection *con;
    bool deleteCon;

  public:
    SocketDebugImpl(Socket *parent) :
      SocketImpl(parent), socketOpen(false), blocking(true),
      listening(false), bound(false), con(0), deleteCon(false) {}
    ~SocketDebugImpl();

    bool isListening() const {return listening;}

    // From SocketImpl
    bool isOpen() const {return socketOpen;}
    void setReuseAddr(bool reuse) {}
    void setBlocking(bool blocking) {this->blocking = blocking;}
    bool getBlocking() const {return blocking;}
    void open() {socketOpen = true;}
    void bind(const IPAddress &ip);
    void listen(int backlog) {open(); listening = true;}
    SmartPointer<Socket> accept(IPAddress *ip);
    void connect(const IPAddress &ip);
    std::streamsize write(const char *data, std::streamsize length,
                          unsigned flags);
    std::streamsize read(char *data, std::streamsize length, unsigned flags);
    void close();
    socket_t get() const {return 0;}
  };
}

#endif // CBANG_SOCKET_DEBUG_IMPL_H

