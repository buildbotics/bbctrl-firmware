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

#ifndef CBANG_HTTP_HANDLER_H
#define CBANG_HTTP_HANDLER_H

#include <string>

#include <cbang/net/URI.h>
#include <cbang/util/Regex.h>


namespace cb {
  namespace HTTP {
    class Context;
    class Connection;

    class Handler {
      bool matchAll;
      Regex re;

    public:
      Handler(const std::string &pattern = "") :
        matchAll(pattern.empty()), re(pattern) {}
      virtual ~Handler() {}

      /// Create a HTTP::Context instance
      virtual Context *createContext(Connection *con);

      /// Build a response from the clients request data
      virtual void buildResponse(Context *ctx) = 0;

      /// Report back to the subclass the if the response was sent
      virtual void reportResults(Context *ctx, bool success) {}

      /// @return True if the client is allowed to access this server
      virtual bool allow(Connection *con) const {return true;}

      virtual bool match(Connection *con) const;
    };
  }
}

#endif // CBANG_HTTP_HANDLER_H
