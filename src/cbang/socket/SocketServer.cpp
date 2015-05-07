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

#include "SocketServer.h"

#include "SocketSet.h"
#include "SocketConnection.h"

#include <cbang/Exception.h>

#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/time/Timer.h>
#include <cbang/openssl/SSLContext.h>

#include <exception>

using namespace std;
using namespace cb;


SocketServer::~SocketServer() {} // Hide destructor


Socket &SocketServer::addListenPort(const IPAddress &ip,
                                    const SmartPointer<SSLContext> &sslCtx) {
  SmartPointer<ListenPort> port = new ListenPort(ip, sslCtx);
  ports.push_back(port);

  return port->socket;
}


Socket &SocketServer::addListenPort(const IPAddress &ip) {
  return addListenPort(ip, 0);
}


bool SocketServer::allow(const IPAddress &clientIP) const {
  return ipFilter.isAllowed(clientIP);
}


void SocketServer::startup() {
  if (ports.empty()) THROW("Server has no listen ports");

  // Open listen ports
  for (unsigned i = 0; i < ports.size(); i++) {
    Socket &socket = ports[i]->socket;

    if (!socket.isOpen()) {
      socket.setReuseAddr(true);
      socket.bind(ports[i]->ip);
      socket.listen();

      // This has to be after bind() and listen() for windows sake
      socket.setBlocking(false);

      LOG_DEBUG(5, "SocketServer bound " << ports[i]->ip);
    }
  }
}


void SocketServer::service() {
  // Add listeners
  SocketSet sockSet;
  for (unsigned i = 0; i < ports.size(); i++)
    sockSet.add(ports[i]->socket, SocketSet::READ);

  // Add others
  addConnectionSockets(sockSet);

  if (sockSet.select(0.1) || connectionsReady()) {
    // Process connections
    processConnections(sockSet);

    // Accept new connections
    for (unsigned i = 0; i < ports.size() && !shouldShutdown(); i++) {
      try {
        Socket &socket = ports[i]->socket;

        if (!sockSet.isSet(socket, SocketSet::READ)) continue;

        IPAddress clientIP;
        while (true) {
          SmartPointer<Socket> client = socket.accept(&clientIP);
          if (client.isNull()) break;

          // Check access
          if (!allow(clientIP.getIP())) {
            LOG_INFO(3, "Server access denied for " << clientIP);
            continue;
          }

          SocketConnectionPtr con = createConnection(client, clientIP);
          con->setIncomingIP(ports[i]->ip);
          client = 0; // Release socket pointer

          if (con.isNull())
            LOG_INFO(3, "Dropped server connection on "
                     << ports[i]->ip << " from " << clientIP);

          else {
            connections.push_back(con);

            LOG_INFO(3, "Server connection id=" << con->getID() << " on "
                     << ports[i]->ip << " from "
                     << IPAddress(clientIP.getIP()));
          }
        }
      } CBANG_CATCH_DEBUG(5);
    }
  }

  cleanupConnections();
}


void SocketServer::shutdown() {
  // Close listen ports
  for (unsigned i = 0; i < ports.size(); i++)
    if (ports[i]->socket.isOpen()) ports[i]->socket.close();

  // Close open connections
  connections_t::iterator it;
  for (it = connections.begin(); it != connections.end(); it++)
    closeConnection(*it);
  connections.clear();
}


void SocketServer::cleanupConnections() {
  // Clean up finished connections
  connections_t::iterator it;
  for (it = connections.begin(); it != connections.end();) {
    const SocketConnectionPtr &con = *it;
    if (con->isFinished()) {
      LOG_INFO(3, "Server connection id=" << con->getID() << " ended");

      closeConnection(con);
      it = connections.erase(it);

    } else it++;
  }
}


void SocketServer::start() {
  startup();
  Thread::start();
}


void SocketServer::run() {
  try {
    Timer timer;

    while (!shouldShutdown()) {
      service();
      timer.throttle(0.01); // Cycle no faster than 100 Hz
    }
  } CBANG_CATCH_ERROR;

  shutdown();
}
