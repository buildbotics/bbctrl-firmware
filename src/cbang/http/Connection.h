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

#ifndef CBANG_HTTP_CONNECTION_H
#define CBANG_HTTP_CONNECTION_H

#include "ConnectionDevice.h"
#include "Request.h"
#include "Response.h"

#include <cbang/buffer/MemoryBuffer.h>
#include <cbang/os/Mutex.h>
#include <cbang/socket/SocketConnection.h>

#include <ostream>
#include <list>

namespace cb {
  class Packet;

  namespace HTTP {
    class Server;
    class Handler;
    class Context;

    class Connection :
      public SocketConnection, public Mutex, public ConnectionStream {
    public:
      enum state_t {
        READING_HEADER,   // 0
        READING_DATA,     // 1
        PROCESSING,       // 2
        DELAY_PROCESSING, // 3
        WRITING_HEADER,   // 4
        WRITING_DATA,     // 5
        REPORTING,        // 6
        CLOSING,          // 7
      };

    protected:
      Server &server;

      Request request;
      Response response;

      MemoryBuffer readBuf;
      MemoryBuffer utilBuf;
      MemoryBuffer dataBuf;
      unsigned contentLength;

      typedef std::list<SmartPointer<Buffer> > responseBuf_t;
      responseBuf_t responseBuf;

      uint64_t lastUpdate;

      int priority;
      state_t state;
      double restartTime;

      Handler *handler;
      Context *ctx;

      bool failed;

    public:
      Connection(Server &server, SmartPointer<Socket> socket,
                 const IPAddress &clientIP);
      virtual ~Connection();

      Request &getRequest() {return request;}
      const Request &getRequest() const {return request;}

      Response &getResponse() {return response;}
      const Response &getResponse() const {return response;}

      void setLastIO(uint64_t lastUpdate) {this->lastUpdate = lastUpdate;}
      uint64_t getLastIO() const {return lastUpdate;}

      int getPriority() const {return priority;}
      void setPriority(int priority) {this->priority = priority;}

      state_t getState() const;
      void setState(state_t state);

      void delayUntil(double restartTime);
      double getRestartTime() const {return restartTime;}

      bool isPersistent() const;
      void setPersistent();

      Handler *getHandler() const {return handler;}
      void setHandler(Handler *handler) {this->handler = handler;}

      Context *getContext() const {return ctx;}
      void setContext(Context *ctx) {this->ctx = ctx;}

      void error(StatusCode reason = StatusCode::HTTP_UNKNOWN);
      void fail(StatusCode reason = StatusCode::HTTP_UNKNOWN);

      bool operator<(const Connection &c) const;

      std::streamsize read(char *s, std::streamsize n);
      std::streamsize write(const char *s, std::streamsize n);

      void readRequest(std::istream &stream);

      void writeResponse(std::ostream &stream);
      void writeRequest(std::ostream &stream);

      MemoryBuffer &getPayload() {return dataBuf;}
      void clearResponseBuffer();
      void addResponseBuffer(const SmartPointer<Buffer> &buf);
      void addResponseBuffer(char *buffer, unsigned length);
      void addResponseBuffer(Packet &packet);
      unsigned getResponseSize() const;

      typedef responseBuf_t::const_iterator iterator;
      iterator responseBufferBegin() const {return responseBuf.begin();}
      iterator responseBufferEnd() const {return responseBuf.end();}

      // Connection state processing functions
      virtual bool readHeader();
      virtual bool read();
      virtual void process();
      virtual bool writeHeader();
      virtual bool write();
      virtual void reportResults();

      virtual std::ostream &print(std::ostream &stream) const;

      // From SocketConnection
      bool isFinished() const {return state == CLOSING;}

    protected:
      unsigned tryParsingHeader(MemoryBuffer &buffer);

      uint64_t readSocket(char *buffer, unsigned length);
      uint64_t writeSocket(const char *buffer, unsigned length);
    };

    inline static
    std::ostream &operator<<(std::ostream &stream, const Connection &con) {
      return con.print(stream);
    }
  }
}

#endif // CBANG_HTTP_CONNECTION_H

