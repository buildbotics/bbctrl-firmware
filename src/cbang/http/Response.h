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

#ifndef CBANG_HTTP_RESPONSE_H
#define CBANG_HTTP_RESPONSE_H

#include "Message.h"
#include "StatusCode.h"

namespace cb {
  class URI;

  namespace HTTP {
    class Response : public Message {
      StatusCode status;
      bool finalized;

    public:
      Response(StatusCode status = StatusCode::HTTP_UNKNOWN,
               float version = 1.1) :
        Message(version), status(status), finalized(false) {}

      void reset();
      void redirect(const URI &uri, StatusCode status =
                    StatusCode::HTTP_TEMPORARY_REDIRECT);

      // From Message
      bool isResponse() const {return true;}
      std::string getHeaderLine() const;
      void readHeaderLine(const std::string &line);

      StatusCode getStatus() const {return status;}
      void setStatus(StatusCode status) {this->status = status;}

      void setCacheExpire(unsigned secs = 60 * 60 * 24 * 10);

      void finalize(unsigned length);
    };
  }
}

#endif // CBANG_HTTP_RESPONSE_H

