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

#ifndef CBANG_SOCKET_SERVER_H
#define CBANG_SOCKET_SERVER_H

#include "Socket.h"
#include "SocketConnection.h"

#include <cbang/os/Thread.h>
#include <cbang/os/Mutex.h>

#include <cbang/net/IPAddressFilter.h>

#include <vector>
#include <list>

namespace cb {
  class SocketSet;
  class SSLContext;

  class SocketServer : public Thread, public Mutex {
  protected:
    struct ListenPort {
      Socket socket;
      IPAddress ip;

      ListenPort(const IPAddress &ip, const SmartPointer<SSLContext> &sslCtx) :
        socket(sslCtx), ip(ip) {}
    };

    typedef std::vector<SmartPointer<ListenPort> > ports_t;
    ports_t ports;

    typedef std::list<SocketConnectionPtr> connections_t;
    connections_t connections;

    IPAddressFilter ipFilter;

  public:
    typedef connections_t::const_iterator iterator;

    virtual ~SocketServer();

    Socket &addListenPort(const IPAddress &ip,
                          const SmartPointer<SSLContext> &sslCtx);
    Socket &addListenPort(const IPAddress &ip);

    IPAddressFilter &getAddressFilter() {return ipFilter;}

    unsigned getNumListenPorts() const {return ports.size();}
    const IPAddress &getListenPort(unsigned i) const {return ports[i]->ip;}
    bool isListenPortSecure(unsigned i) const
    {return ports[i]->socket.isSecure();}

    unsigned getNumConnections() const {return connections.size();}
    iterator begin() const {return connections.begin();}
    iterator end() const {return connections.end();}

    virtual bool allow(const IPAddress &clientIP) const;
    virtual SocketConnectionPtr
    createConnection(SmartPointer<Socket> sock, const IPAddress &clientIP) = 0;
    virtual void addConnectionSockets(SocketSet &sockSet) {}
    virtual bool connectionsReady() const {return false;}
    virtual void processConnections(SocketSet &sockSet) {}
    virtual void closeConnection(const SocketConnectionPtr &con) {};

    // These functions should only be called directly if threading is not used
    virtual void startup();
    virtual void service();
    virtual void shutdown();

    virtual void cleanupConnections();

    // From Thread
    void start();

  protected:
    // From Thread
    void run();
  };
}

#endif // CBANG_SOCKET_SERVER_H
