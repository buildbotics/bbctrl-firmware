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

#ifndef CBANG_DB_NAME_VALUE_TABLE_H
#define CBANG_DB_NAME_VALUE_TABLE_H

#include "Column.h"
#include "Statement.h"

#include <cbang/StdTypes.h>
#include <cbang/SmartPointer.h>

#include <string>

namespace cb {
  namespace DB {
    class Database;

    class NameValueTable {
      Database &db;
      const std::string table;

      SmartPointer<Statement> replaceStmt;
      SmartPointer<Statement> deleteStmt;
      SmartPointer<Statement> selectStmt;

    public:
      NameValueTable(Database &db, const std::string &table);

      void init();
      void create();

      void setf(const char *name, const char *value, ...);
      void set(const std::string &name, const std::string &value);
      void set(const std::string &name, int64_t value);
      void set(const std::string &name, uint64_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, int32_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, uint32_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, int16_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, uint16_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, int8_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, uint8_t value)
      {set(name, (int64_t)value);}
      void set(const std::string &name, double value);
      void set(const std::string &name, float value)
      {set(name, (double)value);}
      void set(const std::string &name);
      void unset(const std::string &name);

      const std::string getString(const std::string &name) const;
      int64_t getInteger(const std::string &name) const;
      double getDouble(const std::string &name) const;

      bool has(const std::string &name) const;

    protected:
      Column doGet(const std::string &name) const;
      void doSet(const std::string &name);
    };
  }
}

#endif // CBANG_DB_NAME_VALUE_TABLE_H

