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

#ifndef CBANG_ITEM
#ifndef CBANG_HTTP_SESSIONS_TABLE_H
#define CBANG_HTTP_SESSIONS_TABLE_H

// Forward declarations
namespace cb {
  namespace HTTP {
    class Session;
  }
}

#define DB_TABLE_NAME Sessions
#define DB_TABLE_NAMESPACE cb
#define DB_TABLE_NAMESPACE2 HTTP
#define DB_TABLE_ROW cb::HTTP::Session
#define DB_TABLE_CONSTRAINTS PRIMARY KEY (id)
#define DB_TABLE_PATH cbang/http

#include <cbang/db/MakeTable.def>

#ifdef DB_TABLE_IMPL
// Implementation includes
#include "Session.h"

#include <cbang/net/IPAddress.h>
#endif // DB_TABLE_IMPL

#endif // CBANG_HTTP_SESSIONS_TABLE_H
#else // CBANG_ITEM

// CBANG_ITEM(Name, DB Type, DB Constraints, cast to DB type, cast from DB type)
CBANG_ITEM(ID,           Text,    NOT NULL, ,)
CBANG_ITEM(User,         Text,    , ,)
CBANG_ITEM(IP,           Integer, , (uint32_t), (cb::IPAddress))
CBANG_ITEM(CreationTime, Integer, , ,)
CBANG_ITEM(LastUsed,     Integer, , ,)

#endif // CBANG_ITEM
