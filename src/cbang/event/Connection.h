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

#ifndef CB_EVENT_CONNECTION_H
#define CB_EVENT_CONNECTION_H

#include <cbang/SmartPointer.h>
#include <cbang/net/IPAddress.h>

struct evhttp_connection;


namespace cb {
  class URI;
  class SSLContext;

  namespace Event {
    class Base;
    class DNSBase;
    class BufferEvent;
    class Request;

    class Connection {
      evhttp_connection *con;
      bool deallocate;

    public:
      Connection(evhttp_connection *con, bool deallocate = true);
      Connection(Base &base, DNSBase &dnsBase, const IPAddress &peer);
      Connection(Base &base, DNSBase &dnsBase, BufferEvent &bev,
                 const IPAddress &peer);
      Connection(Base &base, DNSBase &dnsBase, const URI &uri,
                 const SmartPointer<SSLContext> &sslCtx = 0);
      ~Connection();

      evhttp_connection *getConnection() const {return con;}
      evhttp_connection *adopt() {deallocate = false; return con;}

      cb::IPAddress getPeer() const;

      void setMaxBodySize(unsigned size);
      void setMaxHeaderSize(unsigned size);
      void setInitialRetryDelay(double delay);
      void setRetries(unsigned retries);
      void setTimeout(double timeout);
      void setLocalAddress(const IPAddress &addr);

      void makeRequest(Request &req, unsigned method, const URI &uri);

      void logSSLErrors();
    };
  }
}

#endif // CB_EVENT_CONNECTION_H

