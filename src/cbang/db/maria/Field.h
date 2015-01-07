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

#ifndef CB_MARIADB_FIELD_H
#define CB_MARIADB_FIELD_H

struct st_mysql_field;


namespace cb {
  namespace MariaDB {
    class Field {
      st_mysql_field *field;

    public:
      typedef enum {
        TYPE_DECIMAL,
        TYPE_TINY,
        TYPE_SHORT,
        TYPE_LONG,
        TYPE_FLOAT,
        TYPE_DOUBLE,
        TYPE_NULL,
        TYPE_TIMESTAMP,
        TYPE_LONGLONG,
        TYPE_INT24,
        TYPE_DATE,
        TYPE_TIME,
        TYPE_DATETIME,
        TYPE_YEAR,
        TYPE_NEWDATE,
        TYPE_VARCHAR,
        TYPE_BIT,
        TYPE_NEWDECIMAL  = 246,
        TYPE_ENUM        = 247,
        TYPE_SET         = 248,
        TYPE_TINY_BLOB   = 249,
        TYPE_MEDIUM_BLOB = 250,
        TYPE_LONG_BLOB   = 251,
        TYPE_BLOB        = 252,
        TYPE_VAR_STRING  = 253,
        TYPE_STRING      = 254,
        TYPE_GEOMETRY    = 255,
      } type_t;

      Field(st_mysql_field *field) : field(field) {}

      const char *getName() const;
      const char *getOriginalName() const;
      const char *getTable() const;
      const char *getOriginalTable() const;
      const char *getDBName() const;
      const char *getCatalog() const;
      const char *getDefault() const;
      unsigned getWidth() const;
      unsigned getMaxWidth() const;
      unsigned getDecimals() const;

      type_t getType() const;
      bool isNull() const;
      bool isInteger() const;
      bool isReal() const;
      bool isNumber() const {return isInteger() || isReal();}
      bool isBit() const;
      bool isTime() const;
      bool isBlob() const;
      bool isString() const;
      bool isGeometry() const;
    };
  }
}

#endif // CB_MARIADB_FIELD_H

