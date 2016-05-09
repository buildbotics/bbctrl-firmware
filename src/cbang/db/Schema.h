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

#ifndef CBANG_DB_SCHEMA_H
#define CBANG_DB_SCHEMA_H

#include <cbang/StdTypes.h>

namespace cb {
  namespace DB {
    class Database;

    class Schema {
    public:
      virtual ~Schema() {}

      virtual int64_t getCurrentVersion(Database &db);
      virtual void setCurrentVersion(Database &db, int64_t version);

      void validate(Database &db);

      virtual int64_t getVersion() const = 0;
      virtual void create(Database &db) = 0;
      virtual void upgrade(Database &db, unsigned fromVersion) = 0;
    };
  }
}

#endif // CBANG_DB_SCHEMA_H
