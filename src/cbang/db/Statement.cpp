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

#include "Statement.h"

#include "Database.h"
#include "Blob.h"

#include <cbang/Exception.h>
#include <cbang/json/Sink.h>
#include <cbang/net/Base64.h>
#include <cbang/log/Logger.h>

#include <sqlite3.h>

using namespace std;
using namespace cb;
using namespace cb::DB;


Statement::Statement(Database &db, const string &sql) :
  stmt(0), done(false), validRow(false) {

  LOG_DEBUG(5, "SQL: " << sql);

  if (sqlite3_prepare_v2(db.getDB(), sql.c_str(), sql.length(), &stmt, 0))
    THROWS("Failed to prepare statement: " << sql << ": "
           << sqlite3_errmsg(db.getDB()));
}


Statement::~Statement() {
  if (stmt) {
    sqlite3_finalize(stmt);
    stmt = 0;
  }
}


bool Statement::next() {
  if (done) return false;
  int code = sqlite3_step(stmt);
  validRow = false;

  switch (code) {
  case SQLITE_ROW:
    validRow = true;
    return true;

  case SQLITE_DONE:
    done = true;
    return false;

  default:
    THROWS("Failed to advance statement result: " << Database::errorMsg(code));
  }
}


void Statement::reset() {
  sqlite3_reset(stmt); // Ignore errors
  done = false;
}


void Statement::clearBindings() {
  sqlite3_clear_bindings(stmt);
}


void Statement::execute() {
  while (next()) continue;
  reset();
}


bool Statement::execute(int64_t &result) {
  bool hasResult = next() && columns();
  if (hasResult) result = column(0).toInteger();
  reset();
  return hasResult;
}


bool Statement::execute(double &result) {
  bool hasResult = next() && columns();
  if (hasResult) result = column(0).toDouble();
  reset();
  return hasResult;
}


bool Statement::execute(string &result) {
  bool hasResult = next() && columns();
  if (hasResult) result = column(0).toString();
  reset();
  return hasResult;
}


unsigned Statement::columns() const {
  if (!validRow) return 0;
  return sqlite3_column_count(stmt);
}


Column Statement::column(unsigned i) const {
  return Column(stmt, i);
}


unsigned Statement::parameters() const {
  return sqlite3_bind_parameter_count(stmt);
}


Parameter Statement::parameter(unsigned i) const {
  return Parameter(stmt, i + 1);
}


Parameter Statement::parameter(const std::string &name) const {
  int i = sqlite3_bind_parameter_index(stmt, name.c_str());
  if (!i) THROWS("Invalid parameter '" << name << "' for statement");
  return parameter(i - 1);
}


void Statement::readHeader(JSON::Sink &sink) {
  sink.beginList(true);

  unsigned cols = sqlite3_column_count(stmt);
  for (unsigned i = 0; i < cols; i++)
    sink.append(column(i).getName());

  sink.endList();
}


void Statement::readOne(JSON::Sink &sink) {
  sink.beginList(true);

  unsigned cols = sqlite3_data_count(stmt);
  for (unsigned i = 0; i < cols; i++) {
    Column col = column(i);

    switch (col.getType()) {
    case Column::DB_INTEGER: sink.append(col.toInteger()); break;
    case Column::DB_DOUBLE: sink.append(col.toDouble()); break;
    case Column::DB_STRING: sink.append(col.toString()); break;
    case Column::DB_BLOB: {
      // TODO This could be made more efficient if JSON::Sink would allow
      //   piecewise strings.
      Blob blob = col.toBlob();
      sink.append(Base64().encode((char *)blob.getData(), blob.getLength()));
      break;
    }
    case Column::DB_NULL: sink.appendNull(); break;
    }
  }

  sink.endList();
}


void Statement::readAll(JSON::Sink &sink) {
  sink.beginList();

  sink.beginAppend();
  readHeader(sink);

  while (next()) {
    sink.beginAppend();
    readOne(sink);
  }

  sink.endList();
}
