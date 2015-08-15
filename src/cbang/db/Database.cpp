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

#include "Database.h"

#include "Statement.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/os/SystemUtilities.h>

#include <cbang/log/Logger.h>

#include <sqlite3.h>

#include <stdarg.h>

using namespace std;
using namespace cb;
using namespace cb::DB;


Database::Database(double timeout) : timeout(timeout), db(0), transaction(0) {}


Database::~Database() {
  close();
}


bool Database::isOpen() const {
  return db;
}


void Database::open(const string &filename, unsigned flags) {
  SystemUtilities::ensureDirectory(SystemUtilities::dirname(filename));

  if (sqlite3_open_v2(filename.c_str(), &db, flags, 0)) {
    string err = sqlite3_errmsg(db);
    close();

    THROWS("Failed to open database '" << filename << "': " << err);
  }

  sqlite3_busy_timeout(db, (int)(timeout * 1000));
}


void Database::close() {
  if (isOpen()) {
    if (sqlite3_close(db) != SQLITE_OK)
      LOG_WARNING("Failed to close DB connection: " << lastErrorMsg());
    db = 0;
  }
}


void Database::executef(const char *sql, ...) {
  va_list ap;

  va_start(ap, sql);
  execute(String::vprintf(sql, ap));
  va_end(ap);
}


void Database::execute(const string &sql) {
  // TODO handle SQLITE_LOCKED return code with sqlite3_unlock_notify() and a
  //   condition variable to support shared-cache mode.

  char *err = 0;
  int ret = sqlite3_exec(db, sql.c_str(), 0, 0, &err);

  if (ret || err) {
    string errMsg;

    if (err) {
      errMsg = err;
      sqlite3_free(err);

    } else errMsg = errorMsg(ret);

    THROWS("Error executing: '" << sql << "': " << errMsg);
  }
}


bool Database::execute(const string &sql, int64_t &result) {
  return compile(sql)->execute(result);
}


bool Database::execute(const string &sql, double &result) {
  return compile(sql)->execute(result);
}


bool Database::execute(const string &sql, string &result) {
  return compile(sql)->execute(result);
}


SmartPointer<Statement> Database::compilef(const char *sql, ...) {
  va_list ap;

  va_start(ap, sql);
  SmartPointer<Statement> stmt = compile(String::vprintf(sql, ap));
  va_end(ap);

  return stmt;
}


SmartPointer<Statement> Database::compile(const string &sql) {
  return new Statement(*this, sql);
}


SmartPointer<Transaction> Database::begin(transaction_t type, double timeout) {
  if (transaction) THROW("Already in a transaction");

  switch (type) {
  case DEFERRED: execute("BEGIN DEFERRED"); break;
  case IMMEDIATE: execute("BEGIN IMMEDIATE"); break;
  case EXCLUSIVE: execute("BEGIN EXCLUSIVE"); break;
  }

  return transaction = new Transaction(this, timeout);
}


void Database::commit() {
  if (!transaction) THROW("Not in a transaction");

  execute("COMMIT");

  // NOTE Transaction is deleted by the SmartPointer returned from begin()
  transaction->release();
  transaction = 0;
}


void Database::rollback() {
  if (!transaction) THROW("Not in a transaction");

  execute("ROLLBACK");

  // NOTE Transaction is deleted by the SmartPointer returned from begin()
  transaction->release();
  transaction = 0;
}


SmartPointer<Backup> Database::backup(Database &target) {
  sqlite3_backup *backup = sqlite3_backup_init(target.db, "main", db, "main");
  if (!backup) THROWS("Failed to initialize backup");
  return new Backup(backup);
}


int Database::lastError() const {
  return db ? sqlite3_errcode(db) : 0;
}


const char *Database::lastErrorMsg() const {
  return db ? sqlite3_errmsg(db) : 0;
}


const char *Database::errorMsg(int code) {
  switch (code) {
  case SQLITE_OK:         return "Successful result";
  case SQLITE_ERROR:      return "SQL error or missing database";
  case SQLITE_INTERNAL:   return "Internal logic error in SQLite";
  case SQLITE_PERM:       return "Access permission denied";
  case SQLITE_ABORT:      return "Callback routine requested an abort";
  case SQLITE_BUSY:       return "The database file is locked";
  case SQLITE_LOCKED:     return "A table in the database is locked";
  case SQLITE_NOMEM:      return "A malloc() failed";
  case SQLITE_READONLY:   return "Attempt to write a readonly database";
  case SQLITE_INTERRUPT:  return "Operation terminated by sqlite3_interrupt()";
  case SQLITE_IOERR:      return "Some kind of disk I/O error occurred";
  case SQLITE_CORRUPT:    return "The database disk image is malformed";
  case SQLITE_NOTFOUND:   return "NOT USED. Table or record not found";
  case SQLITE_FULL:       return "Insertion failed because database is full";
  case SQLITE_CANTOPEN:   return "Unable to open the database file";
  case SQLITE_PROTOCOL:   return "NOT USED. Database lock protocol error";
  case SQLITE_EMPTY:      return "Database is empty";
  case SQLITE_SCHEMA:     return "The database schema changed";
  case SQLITE_TOOBIG:     return "String or BLOB exceeds size limit";
  case SQLITE_CONSTRAINT: return "Abort due to constraint violation";
  case SQLITE_MISMATCH:   return "Data type mismatch";
  case SQLITE_MISUSE:     return "Library used incorrectly";
  case SQLITE_NOLFS:      return "Uses OS features not supported on host";
  case SQLITE_AUTH:       return "Authorization denied";
  case SQLITE_FORMAT:     return "Auxiliary database format error";
  case SQLITE_RANGE:      return "2nd parameter to sqlite3_bind out of range";
  case SQLITE_NOTADB:     return "File opened that is not a database file";
  case SQLITE_ROW:        return "sqlite3_step() has another row ready";
  case SQLITE_DONE:       return "sqlite3_step() has finished executing";
  default:                return "UNKNOWN SQLITE error code";
  }
}


string Database::escape(const string &s) {
  char *escaped = sqlite3_mprintf("%q", s.c_str());
  string result(escaped);
  sqlite3_free(escaped);
  return result;
}
