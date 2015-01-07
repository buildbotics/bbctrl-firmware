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

#include "Field.h"

#include <mysql/mysql.h>

using namespace cb::MariaDB;


const char *Field::getName() const {
  return field->name;
}


const char *Field::getOriginalName() const {
  return field->org_name;
}


const char *Field::getTable() const {
  return field->table;
}


const char *Field::getOriginalTable() const {
  return field->org_table;
}


const char *Field::getDBName() const {
  return field->db;
}


const char *Field::getCatalog() const {
  return field->catalog;
}


const char *Field::getDefault() const {
  return field->def;
}


unsigned Field::getWidth() const {
  return field->length;
}


unsigned Field::getMaxWidth() const {
  return field->max_length;
}


unsigned Field::getDecimals() const {
  return field->decimals;
}


Field::type_t Field::getType() const {
  return (type_t)field->type;
}


bool Field::isNull() const {
  return getType() == TYPE_NULL;
}


bool Field::isInteger() const {
  switch (getType()) {
  case TYPE_TINY:
  case TYPE_SHORT:
  case TYPE_LONG:
  case TYPE_LONGLONG:
  case TYPE_INT24:
  case TYPE_YEAR:
    return true;
  default: return false;
  }
}


bool Field::isReal() const {
  switch (getType()) {
  case TYPE_DECIMAL:
  case TYPE_FLOAT:
  case TYPE_DOUBLE:
    return true;
  default: return false;
  }
}


bool Field::isBit() const {
  return getType() == TYPE_BIT;
}


bool Field::isTime() const {
  switch (getType()) {
  case TYPE_TIMESTAMP:
  case TYPE_DATE:
  case TYPE_TIME:
  case TYPE_DATETIME:
  case TYPE_YEAR:
  case TYPE_NEWDATE:
    return true;
  default: return false;
  }
}


bool Field::isBlob() const {
  switch (getType()) {
  case TYPE_TINY_BLOB:
  case TYPE_MEDIUM_BLOB:
  case TYPE_LONG_BLOB:
  case TYPE_BLOB:
    return true;
  default: return false;
  }
}


bool Field::isString() const {
  switch (getType()) {
  case TYPE_VARCHAR:
  case TYPE_VAR_STRING:
  case TYPE_STRING:
  case TYPE_ENUM:
  case TYPE_SET:
    return true;
  default: return false;
  }
}


bool Field::isGeometry() const {
  return getType() == TYPE_GEOMETRY;
}
