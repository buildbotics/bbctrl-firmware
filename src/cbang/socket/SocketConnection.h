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

#ifndef CBANG_SOCKET_CONNECTION_H
#define CBANG_SOCKET_CONNECTION_H

#include "Socket.h"

#include <cbang/SmartPointer.h>

#include <cbang/net/IPAddress.h>

#include <cbang/time/Time.h>
#include <cbang/util/UniqueID.h>

namespace cb {
  class SSL;

  class SocketConnection : protected UniqueID<SocketConnection, 1> {
    unsigned id;

  protected:
    SmartPointer<Socket> socket;
    IPAddress clientIP;
    IPAddress incomingIP;
    const uint64_t startTime;

  public:
    SocketConnection(SmartPointer<Socket> socket, const IPAddress &clientIP) :
      id(getNextID()), socket(socket), clientIP(clientIP),
      startTime(Time::now()) {}
    virtual ~SocketConnection() {}

    unsigned getID() const {return id;}
    Socket &getSocket() const {return *socket;}
    cb::SSL *getSSL() const;
    bool isSecure() const {return getSSL();}
    const IPAddress &getClientIP() const {return clientIP;}
    void setIncomingIP(const IPAddress &ip) {incomingIP = ip;}
    const IPAddress &getIncomingIP() const {return incomingIP;}
    uint64_t getStartTime() const {return startTime;}

    virtual bool isFinished() const = 0;
  };

  typedef SmartPointer<SocketConnection>::Protected SocketConnectionPtr;
}

#endif // CBANG_SOCKET_CONNECTION_H

