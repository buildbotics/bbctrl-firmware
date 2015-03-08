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

#ifndef CB_MARIADB_EVENT_DBCALLBACK_H
#define CB_MARIADB_EVENT_DBCALLBACK_H

#include <cbang/util/MemberFunctor.h>

namespace cb {
  namespace MariaDB {
    class EventDBCallback {
    public:
      typedef enum {
        EVENTDB_ERROR,
        EVENTDB_BEGIN_RESULT,
        EVENTDB_ROW,
        EVENTDB_END_RESULT,
        EVENTDB_RETRY,
        EVENTDB_DONE,
      } state_t;

      virtual ~EventDBCallback() {}

      virtual void operator()(state_t state) = 0;
    };

    CBANG_FUNCTOR1(EventDBFunctor, EventDBCallback, void, operator(),   \
                   EventDBCallback::state_t);
    CBANG_MEMBER_FUNCTOR1(EventDBMemberFunctor, EventDBCallback, void,  \
                          operator(), EventDBCallback::state_t);
  }
}

#endif // CB_MARIADB_EVENT_DBCALLBACK_H

