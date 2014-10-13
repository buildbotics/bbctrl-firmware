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

#ifndef CB_HTTP_WEB_PAGE_HANDLER_GROUP_H
#define CB_HTTP_WEB_PAGE_HANDLER_GROUP_H

#include "WebPageHandler.h"
#include "MethodWebPageHandler.h"


namespace cb {
  namespace HTTP {
    class WebPageHandlerGroup : public WebPageHandler {
      std::vector<SmartPointer<WebPageHandler> > handlers;

    public:
      void addHandler(const SmartPointer<WebPageHandler> &handler);
      void addHandler(const std::string &match,
                      const SmartPointer<WebPageHandler> &handler);

      template <class T>
      void addMethod(T *obj,
                      typename MethodWebPageHandler<T>::method_t method) {
        addHandler(new MethodWebPageHandler<T>(obj, method));
      }

      template <class T>
      void addMethod(const std::string &match, T *obj,
                     typename MethodWebPageHandler<T>::method_t method) {
        addHandler(match, new MethodWebPageHandler<T>(obj, method));
      }

      // From WebPageHandler
      bool handlePage(WebContext &ctx, std::ostream &stream,
                      const cb::URI &uri);
    };
  }
}

#endif // CB_HTTP_WEB_PAGE_HANDLER_GROUP_H

