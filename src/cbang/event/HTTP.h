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

#ifndef CB_EVENT_HTTP_H
#define CB_EVENT_HTTP_H

#include "HTTPHandler.h"

#include <cbang/SmartPointer.h>

#include <vector>

struct evhttp;


namespace cb {
  class SSLContext;
  class IPAddress;

  namespace Event {
    class Base;

    class HTTP {
      evhttp *http;
      SmartPointer<SSLContext> sslCtx;

      std::vector<SmartPointer<HTTPHandler> > requestCallbacks;

    public:
      HTTP(const Base &base, const SmartPointer<SSLContext> &sslCtx = 0);
      virtual ~HTTP();

      void setMaxBodySize(unsigned size);
      void setMaxHeadersSize(unsigned size);
      void setTimeout(int timeout);

      void setCallback(const std::string &path,
                       const SmartPointer<HTTPHandler> &cb);
      void setGeneralCallback(const SmartPointer<HTTPHandler> &cb);

      template <class T>
      void setCallback(const std::string &path, T *obj,
                       typename HTTPHandlerMemberFunctor<T>::member_t member) {
        setCallback(path, new HTTPHandlerMemberFunctor<T>(obj, member));
      }

      template <class T>
      void setGeneralCallback(T *obj, typename HTTPHandlerMemberFunctor<T>::
                              member_t member) {
        setGeneralCallback(new HTTPHandlerMemberFunctor<T>(obj, member));
      }

      int bind(const IPAddress &addr);
    };
  }
}

#endif // CB_EVENT_HTTP_H

