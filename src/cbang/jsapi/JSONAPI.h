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

#ifndef CB_JSAPI_JSONAPI_H
#define CB_JSAPI_JSONAPI_H

#include "Handler.h"
#include "MemberFunctor.h"

#include <cbang/http/WebPageHandler.h>
#include <cbang/SmartPointer.h>

#include <map>
#include <string>


namespace cb {
  namespace JSON {
    class Value;
    class Sink;
  }

  namespace JSAPI {
    class JSONAPI : public HTTP::WebPageHandler {
      std::string root;
      std::string jsonpCB;

      typedef std::map<std::string, SmartPointer<Handler> > api_t;
      api_t api;

    public:
      JSONAPI(const std::string &root,
              const std::string &jsonpCB = std::string()) :
        root(root), jsonpCB(jsonpCB) {}

      void add(const std::string &path,
               const SmartPointer<Handler> &handler);

      template <class T>
      void add(const std::string &path, T *obj,
               typename MemberFunctor<T>::member_t member) {
        add(path, new MemberFunctor<T>(obj, member));
      }

      void dispatch(HTTP::WebContext &ctx, const std::string &cmd,
                    const SmartPointer<JSON::Value> &msg,
                    JSON::Sink &sync) const;

      // From HTTP::WebPageHandler
      bool handlePage(HTTP::WebContext &ctx, std::ostream &stream,
                      const URI &uri);
    };
  }
}

#endif // CB_JSAPI_JSONAPI_H

