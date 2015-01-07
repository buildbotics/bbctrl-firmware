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

#include "Column.h"

#include "Blob.h"

#include <sqlite3.h>

using namespace cb::DB;


Column::type_t Column::getType() const {
  return (type_t)sqlite3_column_type(stmt, i);
}


const char *Column::getDeclType() const {
  return sqlite3_column_decltype(stmt, i);
}


const char *Column::getName() const {
  return sqlite3_column_name(stmt, i);
}


const char *Column::getOrigin() const {
#ifdef SQLITE_ENABLE_COLUMN_METADATA
  return sqlite3_column_origin_name(stmt, i);
#else
  return 0;
#endif
}


const char *Column::getTableName() const {
#ifdef SQLITE_ENABLE_COLUMN_METADATA
  return sqlite3_column_table_name(stmt, i);
#else
  return 0;
#endif
}


const char *Column::getDBName() const {
#ifdef SQLITE_ENABLE_COLUMN_METADATA
  return sqlite3_column_database_name(stmt, i);
#else
  return 0;
#endif
}


Blob Column::toBlob() const {
  const void *blob = sqlite3_column_blob(stmt, i);
  int bytes = 0;
  if (blob) bytes = sqlite3_column_bytes(stmt, i);
  return Blob(blob, bytes);
}


double Column::toDouble() const {
  return sqlite3_column_double(stmt, i);
}


int64_t Column::toInteger() const {
  return sqlite3_column_int64(stmt, i);
}


const char *Column::toString() const {
  const char *result = (char *)sqlite3_column_text(stmt, i);
  return result ? result : "";
}
