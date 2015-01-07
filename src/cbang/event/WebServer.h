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

#ifndef CB_EVENT_WEB_SERVER_H
#define CB_EVENT_WEB_SERVER_H

#include "HTTPHandlerGroup.h"

#include <cbang/net/IPAddressFilter.h>

namespace cb {
  class Options;
  class SSLContext;

  namespace Event {
    class Base;
    class HTTP;
    class Request;

    class WebServer : public HTTPHandlerGroup {
      Options &options;
      SmartPointer<SSLContext> sslCtx;

      SmartPointer<HTTP> http;
      SmartPointer<HTTP> https;

      bool initialized;

      IPAddressFilter ipFilter;

      typedef std::vector<IPAddress> ports_t;
      ports_t ports;
      ports_t securePorts;

    public:
      WebServer(Options &options, const Base &base,
                const SmartPointer<SSLContext> &sslCtx = 0,
                const SmartPointer<HTTPHandlerFactory> &factory =
                new HTTPHandlerFactory);
      virtual ~WebServer() {}

      virtual void init();
      virtual bool allow(Request &req) const;

      // From HTTPHandler
      bool operator()(Request &req);

      void addListenPort(const IPAddress &addr);
      unsigned getNumListenPorts() const {return ports.size();}
      const IPAddress &getListenPort(unsigned i) const {return ports.at(i);}

      void addSecureListenPort(const IPAddress &addr);
      unsigned getNumSecureListenPorts() const {return securePorts.size();}
      const IPAddress &getSecureListenPort(unsigned i) const
      {return securePorts.at(i);}

      void setMaxBodySize(unsigned size);
      void setMaxHeadersSize(unsigned size);
      void setTimeout(int timeout);
    };
  }
}

#endif // CB_EVENT_WEB_SERVER_H

