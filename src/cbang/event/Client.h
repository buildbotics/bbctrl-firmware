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

#ifndef CB_EVENT_CLIENT_H
#define CB_EVENT_CLIENT_H

#include "HTTPHandler.h"

#include <cbang/SmartPointer.h>


namespace cb {
  class URI;
  class SSLContext;

  namespace Event {
    class Base;
    class DNSBase;
    class HTTPHandler;
    class PendingRequest;

    class Client {
      Base &base;
      DNSBase &dns;
      cb::SmartPointer<SSLContext> sslCtx;

    public:
      Client(Base &base, DNSBase &dns,
             const cb::SmartPointer<SSLContext> &sslCtx = 0);
      ~Client();

      Base &getBase() {return base;}
      DNSBase &getDNS() {return dns;}
      const cb::SmartPointer<SSLContext> &getSSLContext() const {return sslCtx;}

      SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method, const char *data,
           unsigned length, const cb::SmartPointer<HTTPHandler> &cb);

      SmartPointer<PendingRequest>
      call(const URI &uri, unsigned method,
           const cb::SmartPointer<HTTPHandler> &cb);

      template<class T>
      SmartPointer<PendingRequest>
      callMember(const URI &uri, unsigned method, T *obj,
                 typename HTTPHandlerMemberFunctor<T>::member_t member) {
        return call(uri, method, new HTTPHandlerMemberFunctor<T>(obj, member));
      }

      template<class T>
      SmartPointer<PendingRequest>
      callMember(const URI &uri, unsigned method, const char *data,
                 unsigned length, T *obj,
                 typename HTTPHandlerMemberFunctor<T>::member_t member) {
        return call(uri, method, data, length,
                    new HTTPHandlerMemberFunctor<T>(obj, member));
      }
    };
  }
}

#endif // CB_EVENT_CLIENT_H

