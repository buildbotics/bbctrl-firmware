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

#ifndef CBANG_DB_DATABASE_H
#define CBANG_DB_DATABASE_H

#include "Transaction.h"
#include "Backup.h"

#include <cbang/SmartPointer.h>

#include <string>

struct sqlite3;

namespace cb {
  namespace DB {
    class Statement;
    class Blob;

    class Database {
      double timeout;
      sqlite3 *db;
      Transaction *transaction;

    public:
      typedef enum {
        DEFERRED,
        IMMEDIATE,
        EXCLUSIVE,
      } transaction_t;

    public:
      typedef enum {
        READ_ONLY     = 0x00000001,
        READ_WRITE    = 0x00000002,
        CREATE        = 0x00000004,
        NO_MUTEX      = 0x00008000,
        FULL_MUTEX    = 0x00010000,
        SHARED_CACHE  = 0x00020000,
        PRIVATE_CACHE = 0x00040000,
      } open_mode_t;

      Database(double timeout = 30);
      virtual ~Database();

      sqlite3 *getDB() const {return db;}

      bool isOpen() const;

      void open(const std::string &con, unsigned flags = READ_WRITE | CREATE);
      void close();

      void executef(const char *sql, ...);
      void execute(const std::string &sql);
      bool execute(const std::string &sql, int64_t &result);
      bool execute(const std::string &sql, double &result);
      bool execute(const std::string &sql, std::string &result);

      SmartPointer<Statement> compilef(const char *sql, ...);
      SmartPointer<Statement> compile(const std::string &sql);

      SmartPointer<Transaction> begin(transaction_t type = DEFERRED);
      void commit();
      void rollback();

      SmartPointer<Backup> backup(Database &target);

      int lastError() const;
      const char *lastErrorMsg() const;

      static const char *errorMsg(int code);
    };
  }
}

#endif // CBANG_DB_DATABASE_H

