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

#ifndef CBANG_HTTP_CONTEXT_H
#define CBANG_HTTP_CONTEXT_H

#include "Connection.h"

namespace cb {
  namespace HTTP {
    class Context {
    protected:
      Connection &con;

    public:
      Context(Connection &con) : con(con) {}
      virtual ~Context() {}

      Connection &getConnection() const {return con;}
      const IPAddress &getClientIP() const {return con.getClientIP();}
      const IPAddress &getIncommingIP() const {return con.getIncommingIP();}
      Request &getRequest() const {return con.getRequest();}
      Response &getResponse() const {return con.getResponse();}
      const URI &getURI() const {return getRequest().getURI();}
      cb::SSL *getSSL() const {return con.getSSL();}

      /// A sub-class of Context can return false from
      /// Context::isReady() in order to put a connection on hold
      virtual bool isReady() const {return true;}
    };
  }
}

#endif // CBANG_HTTP_CONTEXT_H

