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

#ifndef CB_EVENT_FILE_HANDLER_H
#define CB_EVENT_FILE_HANDLER_H

#include "HTTPHandler.h"

#include <cbang/time/Time.h>

#include <string>


namespace cb {
  namespace Event {
    class FileHandler : public HTTPHandler {
      std::string root;
      uint64_t timeout;
      bool directory;

    public:
      FileHandler(const std::string &root,
                  uint64_t timeout = Time::SEC_PER_HOUR);

      void setTimeout(uint64_t timeout) {this->timeout = timeout;}
      uint64_t getTimeout() const {return timeout;}

      // From HTTPHandler
      bool operator()(Request &req);
    };
  }
}

#endif // CB_EVENT_FILE_HANDLER_H

