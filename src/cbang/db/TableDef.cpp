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

#include "TableDef.h"

#include "Database.h"

#include <cbang/String.h>
#include <cbang/Exception.h>

#include <cbang/util/DefaultCatch.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::DB;


void TableDef::add(const ColumnDef &column) {
  columnMap[column.getName()] = columns.size();
  columns.push_back(column);
}


void TableDef::add(const char *columns[][3]) {
  for (unsigned i = 0; columns[i][0] && columns[i][1] && columns[i][2]; i++)
    add(ColumnDef(columns[i][0], columns[i][1], columns[i][2]));
}


unsigned TableDef::getIndex(const std::string &column) const {
  columnMap_t::const_iterator it = columnMap.find(column);
  if (it == columnMap.end())
    THROWS("Index for column '" << column << "' not found");
  return it->second;
}


void TableDef::create(Database &db) {
  ostringstream sql;

  sql << "CREATE TABLE IF NOT EXISTS \"" << name << "\" (";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ",";
    sql << '"' << it->getName() << "\" " << it->getType() << " "
        << it->getConstraints();
  }

  if (constraints != "") {
    if (!columns.empty()) sql << ",";
    sql << constraints;
  }

  sql << ")";

  db.execute(sql.str());
}


void TableDef::rebuild(Database &db, const columnRemap_t &_columnRemap) {
  // Parse existing structure
  string result;
  if (!db.execute(string("SELECT sql FROM sqlite_master WHERE name=\"") + name +
                  "\" AND type=\"table\"", result))
    THROWS("Failed to read " << name << " table structure");

  {
    size_t start = result.find('(');
    size_t end = result.find_last_of(')');
    if (start == string::npos || end == string::npos)
      THROWS("Failed to parse " << name << " table structure: " << result);

    result = result.substr(start + 1, end - start - 1);
  }

  vector<string> cols;
  String::tokenize(result, cols, ",");

  // Remap columns
  CIStringMap columnRemap(_columnRemap);
  for (unsigned i = 0; i < cols.size() - 1; i++) {
    size_t length = cols[i].find(' ');
    if (length == string::npos)
      THROWS("Failed to parse " << name << " table structure at: " << cols[i]);

    string col = String::trim(cols[i].substr(0, length), "\"");

    if (columnRemap.find(col) == columnRemap.end())
      columnRemap[col] = col;
  }

  // Create SQL copy command
  string sql = string("INSERT INTO \"") + name + "\" SELECT ";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql += ", ";

    columnRemap_t::iterator it2 = columnRemap.find(it->getName());
    if (it2 == columnRemap.end()) {
      string type = String::toLower(it->getType());

      if (type == "text") sql += "\"\"";
      else if (type == "blob") sql += "NULL";
      else sql += "0";

    } else sql += string("\"") + it2->second + "\"";
  }

  sql += " FROM \"" + name + "_Temp\"";

  // Move table to temp
  db.execute(string("ALTER TABLE \"") + name + "\" RENAME TO \"" + name +
             "_Temp\"");

  // Recreated table
  create(db);

  // Copy data
  try {
    db.execute(sql);
  } CBANG_CATCH_ERROR;

  // Delete temp
  db.execute(string("DROP TABLE \"") + name + "_Temp\"");
}


void TableDef::deleteAll(Database &db) {
  db.execute(string("DELETE FROM \"") + name + "\"");
}


SmartPointer<Statement>
TableDef::makeWriteStmt(Database &db, const string &suffix) {
  ostringstream sql;

  sql << "REPLACE INTO \"" << name << "\" (";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ", ";
    sql << '"' << it->getName() << '"';
  }

  sql << ") VALUES (";

  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ", ";

    sql << "@" << String::toUpper(it->getName());
  }

  sql << ") " << suffix;

  return db.compile(sql.str());
}


SmartPointer<Statement>
TableDef::makeReadStmt(Database &db, const string &suffix) {
  ostringstream sql;

  sql << "SELECT ";

  // This is actually faster than '*' and guarantees the correct order
  for (iterator it = begin(); it != end(); it++) {
    if (it != begin()) sql << ", ";
    sql << '"' << it->getName() << '"';
  }

  sql << "FROM \"" << name << "\" " << suffix;

  return db.compile(sql.str());
}
