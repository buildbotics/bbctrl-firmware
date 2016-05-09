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

#ifndef CBANG_HTTP_SERVER_H
#define CBANG_HTTP_SERVER_H

#include "ConnectionQueue.h"

#include <cbang/socket/SocketServer.h>
#include <cbang/os/ThreadPool.h>
#include <cbang/net/URI.h>
#include <cbang/net/IPAddressFilter.h>

#include <vector>
#include <list>


namespace cb {
  class SSLContext;
  class Options;

  namespace HTTP {
    class Handler;
    class Connection;

    class Server : public SocketServer {
    protected:
      Options &options;
      SmartPointer<SSLContext> sslCtx;
      bool initialized;

      ConnectionQueue queue;
      bool queueConnections;

      SmartPointer<ThreadPool> pool;

      typedef std::vector<Handler *> handlers_t;
      handlers_t handlers;

      IPAddressFilter ipFilter;

      unsigned maxRequestLength;
      unsigned maxConnections;
      unsigned connectionTimeout;
      unsigned maxConnectTime;
      unsigned minConnectTime;

      bool captureRequests;
      bool captureResponses;
      bool captureOnError;
      std::string captureDir;

    public:
      Server(Options &options);
      Server(Options &options, SmartPointer<SSLContext> sslCtx);
      ~Server();

      const SmartPointer<SSLContext> &getSSLContext() const {return sslCtx;}

      virtual const std::string getName() const {return "HTTP::Server";}
      bool getQueueConnections() const {return queueConnections;}
      void setQueueConnections(bool x) {queueConnections = x;}
      unsigned getMaxRequestLength() const {return maxRequestLength;}

      void addHandler(Handler *handler) {handlers.push_back(handler);}
      Handler *match(Connection *con);

      void captureRequest(Connection *con);
      void captureResponse(Connection *con);

      virtual void init();

      bool handleConnection(double timeout);

      void createThreadPool(unsigned size);
      void startThreadPool() {if (!pool.isNull()) pool->start();}
      void stopThreadPool() {if (!pool.isNull()) pool->stop();}
      void joinThreadPool() {if (!pool.isNull()) pool->join();}

      // From Thread
      void start();
      void stop();
      void join();

    protected:
      void construct();

      void process(Connection &con);
      void poolThread();

      void processConnection(const SocketConnectionPtr &con, bool ready);
      void limitConnections();

      // From SocketServer
      SocketConnectionPtr
      createConnection(SmartPointer<Socket> socket, const IPAddress &clientIP);
      void addConnectionSockets(SocketSet &sockSet);
      bool connectionsReady() const;
      void processConnections(SocketSet &sockSet);
      void closeConnection(const SocketConnectionPtr &con);

      std::string getCaptureFilename(const std::string &name, unsigned id);
    };
  }
}

#endif // CBANG_HTTP_SERVER_H
