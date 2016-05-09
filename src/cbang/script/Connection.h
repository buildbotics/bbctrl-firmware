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

#ifndef CBANG_SCRIPT_CONNECTION_H
#define CBANG_SCRIPT_CONNECTION_H

#include "Processor.h"

#include <cbang/socket/SocketConnection.h>

#include <cbang/os/Thread.h>

namespace cb {
  namespace Script {
    class Server;

    class Connection :
      public SocketConnection, public Thread, public Processor {
      Server *server;

    public:
      Connection(Server *server, SmartPointer<Socket> socket,
                 const IPAddress &clientIP) :
        SocketConnection(socket, clientIP), server(server) {}

      // From SocketConnection
      bool isFinished() const;
      using SocketConnection::getID;

      // From Thread
      void run();
      void stop();
    };
  }
}

#endif // CBANG_SCRIPT_CONNECTION_H
