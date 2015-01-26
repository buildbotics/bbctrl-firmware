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

#include "DB.h"

#include <cbang/String.h>
#include <cbang/Exception.h>
#include <cbang/time/Time.h>
#include <cbang/json/Sync.h>
#include <cbang/json/Dict.h>
#include <cbang/log/Logger.h>

#include <mysql/mysql.h>

#include <string.h>

#ifdef WIN32
#define STR_DATA(s) (s).c_str()
#else
#define STR_DATA(s) (s).data()
#endif

using namespace std;
using namespace cb;
using namespace cb::MariaDB;


DB::DB(st_mysql *db) :
  db(db ? db : mysql_init(0)), res(0), nonBlocking(false), connected(false),
  stored(false), status(0), continueFunc(0) {
  if (!this->db) THROW("Failed to create MariaDB");
}


DB::~DB() {
  if (db) mysql_close(db);
}


void DB::setInitCommand(const string &cmd) {
  if (mysql_options(db, MYSQL_INIT_COMMAND, cmd.c_str()))
    THROWS("Failed to set MariaDB init command: " << cmd);
}


void DB::enableCompression() {
  if (mysql_options(db, MYSQL_OPT_COMPRESS, 0))
    THROW("Failed to enable MariaDB compress");
}


void DB::setConnectTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &secs))
    THROWS("Failed to set MariaDB connect timeout to " << secs);
}


void DB::setLocalInFile(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_OPT_LOCAL_INFILE, &x))
    THROWS("Failed to " << (enable ? "enable" : "disable")
           << " MariaD local infile");
}


void DB::enableNamedPipe() {
  if (mysql_options(db, MYSQL_OPT_NAMED_PIPE, 0))
    THROWS("Failed to enable MariaDB named pipe");
}


void DB::setProtocol(protocol_t protocol) {
  mysql_protocol_type type;
  switch (protocol) {
  case PROTOCOL_TCP: type = MYSQL_PROTOCOL_TCP; break;
  case PROTOCOL_SOCKET: type = MYSQL_PROTOCOL_SOCKET; break;
  case PROTOCOL_PIPE: type = MYSQL_PROTOCOL_PIPE; break;
  default: THROWS("Invalid protocol " << protocol);
  }

  if (mysql_options(db, MYSQL_OPT_PROTOCOL, &type))
    THROWS("Failed to set MariaDB protocol to " << protocol);
}


void DB::setReconnect(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_OPT_RECONNECT, &x))
    THROWS("Failed to " << (enable ? "enable" : "disable")
           << "MariaDB auto reconnect");
}


void DB::setReadTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &secs))
    THROWS("Failed to set MariaDB read timeout to " << secs);
}


void DB::setWriteTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &secs))
    THROWS("Failed to set MariaDB write timeout to " << secs);
}


void DB::setDefaultFile(const string &path) {
  if (mysql_options(db, MYSQL_READ_DEFAULT_FILE, path.c_str()))
    THROWS("Failed to set MariaDB default type to " << path);
}


void DB::readDefaultGroup(const string &path) {
  if (mysql_options(db, MYSQL_READ_DEFAULT_GROUP, path.c_str()))
    THROWS("Failed to read MariaDB default group file " << path);
}


void DB::setReportDataTruncation(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_REPORT_DATA_TRUNCATION, &x))
    THROWS("Failed to" << (enable ? "enable" : "disable")
           << " MariaDB data truncation reporting.");
}


void DB::setCharacterSet(const string &name) {
  if (mysql_options(db, MYSQL_SET_CHARSET_NAME, name.c_str()))
    THROWS("Failed to set MariaDB character set to " << name);
}


void DB::enableNonBlocking() {
  if (mysql_options(db, MYSQL_OPT_NONBLOCK, 0))
    THROW("Failed to set MariaDB to non-blocking mode");
  nonBlocking = true;
}


void DB::connect(const string &host, const string &user, const string &password,
                 const string &dbName, unsigned port, const string &socketName,
                 flags_t flags) {
  assertNotPending();
  MYSQL *db = mysql_real_connect
    (this->db, host.c_str(), user.c_str(), password.c_str(), dbName.c_str(),
     port, socketName.empty() ? 0 : socketName.c_str(), flags);

  if (!db) raiseError("Failed to connect");
  connected = true;
}


bool DB::connectNB(const string &host, const string &user,
                   const string &password, const string &dbName, unsigned port,
                   const string &socketName, flags_t flags) {
  LOG_DEBUG(5, __func__ << "()");

  assertNotPending();
  assertNonBlocking();

  MYSQL *db = 0;
  status = mysql_real_connect_start
    (&db, this->db, host.c_str(), user.c_str(), password.c_str(),
     dbName.c_str(), port, socketName.empty() ? 0 : socketName.c_str(), flags);

  if (status) {
    continueFunc = &DB::connectContinue;
    return false;
  }

  if (!db) raiseError("Failed to connect");
  connected = true;

  return true;
}


void DB::close() {
  assertConnected();

  if (db) {
    mysql_close(db);
    db = 0;
  }
  connected = false;
}


bool DB::closeNB() {
  LOG_DEBUG(5, __func__ << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  status = mysql_close_start(db);
  if (status) {
    continueFunc = &DB::closeContinue;
    return false;
  }

  connected = false;
  return true;
}


void DB::use(const string &dbName) {
  assertConnected();
  assertNotPending();

  if (mysql_select_db(db, dbName.c_str()))
    raiseError("Failed to select DB");
}


bool DB::useNB(const string &dbName) {
  LOG_DEBUG(5, __func__ << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_select_db_start(&ret, db, dbName.c_str());

  if (status) {
    continueFunc = &DB::useContinue;
    return false;
  }

  if (ret) raiseError("Failed to select DB");

  return true;
}


void DB::query(const string &s) {
  assertConnected();
  assertNotPending();

  if (mysql_real_query(db, STR_DATA(s), s.length()))
    raiseError("Query failed");
}


bool DB::queryNB(const string &s) {
  assertConnected();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_real_query_start(&ret, db, STR_DATA(s), s.length());

  LOG_DEBUG(5, __func__ << "() status=" << status << " ret=" << ret);

  if (status) {
    continueFunc = &DB::queryContinue;
    return false;
  }

  if (ret) raiseError("Query failed");

  return true;
}


void DB::useResult() {
  assertConnected();
  assertNotPending();
  assertNotHaveResult();

  res = mysql_use_result(db);
  if (!res) THROW("Failed to use result");

  stored = false;
}


void DB::storeResult() {
  assertConnected();
  assertNotPending();
  assertNotHaveResult();

  res = mysql_store_result(db);

  if (res) stored = true;
  else if (hasError()) raiseError("Failed to store result");
}


bool DB::storeResultNB() {
  LOG_DEBUG(5, __func__ << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();
  assertNotHaveResult();

  status = mysql_store_result_start(&res, db);
  if (status) {
    continueFunc = &DB::storeResultContinue;
    return false;
  }

  if (res) stored = true;
  else if (hasError()) raiseError("Failed to store result");

  return true;
}


bool DB::haveResult() const {
  return res;
}


bool DB::nextResult() {
  assertConnected();
  assertNotHaveResult();
  assertNotPending();

  int ret = mysql_next_result(db);
  if (0 < ret) raiseError("Failed to get next result");

  return ret == 0;
}


bool DB::nextResultNB() {
  LOG_DEBUG(5, __func__ << "()");

  assertConnected();
  assertNotHaveResult();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_next_result_start(&ret, db);
  if (status) {
    continueFunc = &DB::nextResultContinue;
    return false;
  }

  if (0 < ret) raiseError("Failed to get next result");

  return true;
}


bool DB::moreResults() const {
  assertConnected();
  return mysql_more_results(db);
}


void DB::freeResult() {
  assertNotPending();
  assertHaveResult();
  mysql_free_result(res);
  res = 0;
  stored = false;
}


bool DB::freeResultNB() {
  LOG_DEBUG(5, __func__ << "()");

  assertNotPending();
  assertNonBlocking();
  assertHaveResult();

  status = mysql_free_result_start(res);
  if (status) {
    continueFunc = &DB::freeResultContinue;
    return false;
  }

  res = 0;

  return true;
}


uint64_t DB::getRowCount() const {
  assertHaveResult();
  return mysql_num_rows(res);
}


uint64_t DB::getAffectedRowCount() const {
  assertHaveResult();
  return mysql_affected_rows(db);
}


unsigned DB::getFieldCount() const {
  assertHaveResult();
  return mysql_num_fields(res);
}


bool DB::fetchRow() {
  assertNotPending();
  assertHaveResult();
  return mysql_fetch_row(res);
}


bool DB::fetchRowNB() {
  LOG_DEBUG(5, __func__ << "()");

  assertNotPending();
  assertNonBlocking();
  assertHaveResult();

  MYSQL_ROW row = 0;
  status = mysql_fetch_row_start(&row, res);
  if (status) {
    continueFunc = &DB::fetchRowContinue;
    return false;
  }

  return true;
}


bool DB::haveRow() const {
  return res && res->current_row;
}


void DB::seekRow(uint64_t row) {
  assertHaveResult();
  if (!stored) THROW("Must use storeResult() before seekRow()");
  if (mysql_num_rows(res) <= row) THROWS("Row seek out of range " << row);
  mysql_data_seek(res, row);
}


void DB::appendRow(JSON::Sync &sync, int first, int count) const {
  for (unsigned i = first; i < getFieldCount() && count; i++, count--) {
    sync.beginAppend();
    writeField(sync, i);
  }
}


void DB::insertRow(JSON::Sync &sync, int first, int count,
                   bool withNulls) const {

  for (unsigned i = first; i < getFieldCount() && count; i++, count--) {
    if (!withNulls && getNull(i)) continue;
    sync.beginInsert(getField(i).getName());
    writeField(sync, i);
  }
}


void DB::writeRowList(JSON::Sync &sync, int first, int count) const {
  sync.beginList();
  appendRow(sync, first, count);
  sync.endList();
}


void DB::writeRowDict(JSON::Sync &sync, int first, int count,
                      bool withNulls) const {
  sync.beginDict();
  insertRow(sync, first, count, withNulls);
  sync.endDict();
}


Field DB::getField(unsigned i) const {
  assertInFieldRange(i);
  return &mysql_fetch_fields(res)[i];
}


Field::type_t DB::getType(unsigned i) const {
  return getField(i).getType();
}


unsigned DB::getLength(unsigned i) const {
  assertInFieldRange(i);
  return mysql_fetch_lengths(res)[i];
}


const char *DB::getData(unsigned i) const {
  assertInFieldRange(i);
  return res->current_row[i];
}


void DB::writeField(JSON::Sync &sync, unsigned i) const {
  if (getNull(i)) sync.writeNull();
  else {
    Field field = getField(i);

    if (field.isNumber()) sync.write(getDouble(i));
    else sync.write(getString(i));
  }
}


bool DB::getNull(unsigned i) const {
  return !res->current_row[i];
}


string DB::getString(unsigned i) const {
  unsigned length = getLength(i);
  return string(res->current_row[i], length);
}


bool DB::getBoolean(unsigned i) const {
  return String::parseBool(getString(i));
}


double DB::getDouble(unsigned i) const {
  if (!getField(i).isNumber()) THROWS("Field " << i << " is not a number");
  return String::parseDouble(getString(i));
}


uint32_t DB::getU32(unsigned i) const {
  if (!getField(i).isInteger()) THROWS("Field " << i << " is not an integer");
  return String::parseU32(getString(i));
}


int32_t DB::getS32(unsigned i) const {
  if (!getField(i).isInteger()) THROWS("Field " << i << " is not an integer");
  return String::parseS32(getString(i));
}


uint64_t DB::getU64(unsigned i) const {
  if (!getField(i).isInteger()) THROWS("Field " << i << " is not an integer");
  return String::parseU64(getString(i));
}


int64_t DB::getS64(unsigned i) const {
  if (!getField(i).isInteger()) THROWS("Field " << i << " is not an integer");
  return String::parseS64(getString(i));
}


uint64_t DB::getBit(unsigned i) const {
  if (getType(i) != Field::TYPE_BIT) THROWS("Field " << i << " is not bit");

  uint64_t x = 0;
  for (const char *ptr = res->current_row[i]; *ptr; ptr++) {
    x <<= 1;
    if (*ptr == '1') x |= 1;
  }

  return x;
}


void DB::getSet(unsigned i, set<string> &s) const {
  if (getType(i) != Field::TYPE_SET) THROWS("Field " << i << " is not a set");

  const char *start = res->current_row[i];
  const char *end = start;

  while (*end) {
    if (*end == ',') {
      s.insert(string(start, end - start));
      start = end + 1;
    }
    end++;
  }
}


double DB::getTime(unsigned i) const {
  assertInFieldRange(i);

  char *s = res->current_row[i];
  unsigned len = res->lengths[i];

  // Parse decimal part
  double decimal = 0;
  char *ptr = strchr(s, '.');
  if (ptr) {
    decimal = String::parseDouble(ptr);
    len = ptr - s;
  }

  string time = string(s, len);

  switch (getType(i)) {
  case Field::TYPE_YEAR:
    if (len == 2) return decimal + Time::parse(time, "%y");
    return decimal + Time::parse(time, "%Y");

  case Field::TYPE_DATE:
    return decimal + Time::parse(time, "%Y-%m-%d");

  case Field::TYPE_TIME:
    return decimal + Time::parse(time, "%H%M%S");

  case Field::TYPE_TIMESTAMP:
  case Field::TYPE_DATETIME:
    return decimal + Time::parse(time, "%Y-%m-%d %H%M%S");

  default: THROWS("Invalid time type");
  }
}


string DB::rowToString() const {
  ostringstream str;

  for (unsigned i = 0; i < getFieldCount(); i++) {
    if (i) str << ", ";
    str << getString(i);
  }

  return str.str();
}


string DB::getInfo() const {
  return mysql_info(db);
}


const char *DB::getSQLState() const {
  return mysql_sqlstate(db);
}


bool DB::hasError() const {
  return mysql_errno(db);
}


string DB::getError() const {
  return mysql_error(db);
}


unsigned DB::getErrorNumber() const {
  return mysql_errno(db);
}


void DB::raiseError(const string &msg) const {
  THROWS("MariaDB: " << msg << ": " << getError());
}


unsigned DB::getWarningCount() const {
  return mysql_warning_count(db);
}


void DB::assertConnected() const {
  if (!connected) THROW("Not connected");
}


void DB::assertPending() const {
  if (!nonBlocking || !status) THROW("Connection not pending");
}


void DB::assertNotPending() const {
  if (status) raiseError("Non-blocking call still pending");
}


void DB::assertNonBlocking() const {
  if (!nonBlocking) raiseError("Connection is not in nonBlocking mode");
}


void DB::assertHaveResult() const {
  if (!haveResult()) raiseError("Don't have result, must call query() and "
                                "useResult() or storeResult()");
}


void DB::assertNotHaveResult() const {
  if (haveResult()) raiseError("Already have result, must call freeResult()");
}


void DB::assertHaveRow() const {
  if (!haveRow()) raiseError("Don't have row, must call fetchRow()");
}


void DB::assertInFieldRange(unsigned i) const {
  if (getFieldCount() <= 0) THROWS("Out of field range " << i);
}


bool DB::continueNB(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  assertPending();
  if (!continueFunc) THROWS("Continue function not set");
  bool ret = (this->*continueFunc)(ready);
  if (ret) continueFunc = 0;
  return ret;
}


bool DB::waitRead() const {
  return status & MYSQL_WAIT_READ;
}


bool DB::waitWrite() const {
  return status & MYSQL_WAIT_WRITE;
}


bool DB::waitExcept() const {
  return status & MYSQL_WAIT_EXCEPT;
}


bool DB::waitTimeout() const {
  return status & MYSQL_WAIT_TIMEOUT;
}


int DB::getSocket() const {
  return mysql_get_socket(db);
}


double DB::getTimeout() const {
  return (double)mysql_get_timeout_value_ms(db) / 1000.0; // millisec -> sec
}


string DB::escape(const string &s) const {
  SmartPointer<char>::Array to = new char[s.length() * 2 + 1];

  unsigned len =
    mysql_real_escape_string(db, to.get(), STR_DATA(s), s.length());

  return string(to.get(), len);
}


namespace {
  string::const_iterator parseSubstitution(string &result, const DB &db,
                                           const JSON::Dict &dict,
                                           string::const_iterator start,
                                           string::const_iterator end) {
    string::const_iterator it = start + 1;

    string name;
    while (it != end && *it != ')') name.push_back(*it++);

    if (it == end || ++it == end) return start;

    if (!dict.has(name)) result.append("null");
    else switch (*it) {
      case 'b': result.append(dict.getBoolean(name) ? "true" : "false"); break;
      case 'f': result.append(String(dict.getNumber(name))); break;
      case 'i': result.append(String(dict.getS32(name))); break;
      case 's': result.append("'" + db.escape(dict.getString(name)) + "'");
        break;
      case 'u': result.append(String(dict.getU32(name))); break;
      default: return start;
      }

    return ++it;
  }
}


string DB::format(const string &s, const JSON::Dict &dict) const {
  string result;
  result.reserve(s.length());

  bool escape = false;

  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    if (escape) {
      escape  = false;
      if (*it == '(') {
        it = parseSubstitution(result, *this, dict, it, s.end());
        if (it == s.end()) break;
      }

    } else if (*it == '%') {
      escape = true;
      continue;
    }

    result.push_back(*it);
  }

  return result;
}


string DB::toHex(const string &s) {
  SmartPointer<char>::Array to = new char[s.length() * 2 + 1];

  unsigned len = mysql_hex_string(to.get(), STR_DATA(s), s.length());

  return string(to.get(), len);
}


void DB::libraryInit(int argc, char *argv[], char *groups[]) {
  if (mysql_library_init(argc, argv, groups)) THROW("Failed to init MariaDB");
}


void DB::libraryEnd() {
  mysql_library_end();
}


const char *DB::getClientInfo() {
  return mysql_get_client_info();
}


void DB::threadInit() {
  if (mysql_thread_init()) THROW("Failed to init MariaDB threads");
}


void DB::threadEnd() {
  mysql_thread_end();
}


bool DB::threadSafe() {
  return mysql_thread_safe();
}


bool DB::closeContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  status = mysql_close_cont(this->db, ready);
  if (status) return false;

  connected = false;
  return true;
}


bool DB::connectContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  MYSQL *db = 0;
  status = mysql_real_connect_cont(&db, this->db, ready);
  if (status) return false;

  if (!db) THROW("Failed to connect");
  connected = true;

  return true;
}


bool DB::useContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  int ret = 0;
  status = mysql_select_db_cont(&ret, this->db, ready);
  if (status) return false;

  if (ret) THROW("Failed to select DB");

  return true;
}


bool DB::queryContinue(unsigned ready) {
  int ret = 0;
  status = mysql_real_query_cont(&ret, this->db, ready);

  LOG_DEBUG(5, __func__ << "() ready=" << ready << " status=" << status
            << " ret=" << ret);

  if (status) return false;
  if (ret) THROW("Query failed");

  return true;
}


bool DB::storeResultContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  status = mysql_store_result_cont(&res, this->db, ready);
  if (status) return false;

  if (res) stored = true;
  else if (hasError()) THROW("Failed to store result");

  return true;
}


bool DB::nextResultContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  int ret = 0;
  status = mysql_next_result_cont(&ret, this->db, ready);
  if (status) return false;

  if (0 < ret) THROW("Failed to get next result");

  return true;
}


bool DB::freeResultContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  status = mysql_free_result_cont(res, ready);
  if (status) return false;

  res = 0;

  return true;
}


bool DB::fetchRowContinue(unsigned ready) {
  LOG_DEBUG(5, __func__ << "()");

  MYSQL_ROW row = 0;
  status = mysql_fetch_row_cont(&row, res, ready);

  return !status;
}
