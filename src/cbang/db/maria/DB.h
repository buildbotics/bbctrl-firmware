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

#ifndef CB_MARIADB_DB_H
#define CB_MARIADB_DB_H

#include "Field.h"

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>

#include <string>
#include <vector>
#include <set>

struct st_mysql;
struct st_mysql_res;


namespace cb {
  namespace JSON {
    class Sync;
    class Dict;
  }

  namespace MariaDB {
    class DB {
    public:
      typedef enum {
        PROTOCOL_TCP,
        PROTOCOL_SOCKET,
        PROTOCOL_PIPE,
      } protocol_t;

      typedef enum {
        FLAG_NONE              = 0,
        FLAG_LONG_PASSWORD     = 1 << 0,
        FLAG_FOUND_ROWS        = 1 << 1,
        FLAG_LONG_FLAG         = 1 << 2,
        FLAG_CONNECT_WITH_DB   = 1 << 3,
        FLAG_NO_SCHEMA         = 1 << 4,
        FLAG_COMPRESS          = 1 << 5,
        FLAG_ODBC              = 1 << 6,
        FLAG_LOCAL_FILES       = 1 << 7,
        FLAG_IGNORE_SPACE      = 1 << 8,
        FLAG_PROTOCOL_41       = 1 << 9,
        FLAG_INTERACTIVE       = 1 << 10,
        FLAG_SSL               = 1 << 11,
        FLAG_IGNORE_SIGPIPE    = 1 << 12,
        FLAG_TRANSACTIONS      = 1 << 13,
        FLAG_RESERVED          = 1 << 14,
        FLAG_SECURE_CONNECTION = 1 << 15,
        FLAG_MULTI_STATEMENTS  = 1 << 16,
        FLAG_MULTI_RESULTS     = 1 << 17,
        FLAG_PS_MULTI_RESULTS  = 1 << 18,
        FLAG_DEFAULTS = FLAG_MULTI_STATEMENTS | FLAG_MULTI_RESULTS,
      } flags_t;

      typedef enum {
        READY_NONE    = 0,
        READY_READ    = 1 << 0,
        READY_WRITE   = 1 << 1,
        READY_EXCEPT  = 1 << 2,
        READY_TIMEOUT = 1 << 3,
      } ready_t;

    protected:
      st_mysql *db;
      st_mysql_res *res;

      typedef bool (DB::*continue_func_t)(unsigned ready);

      bool nonBlocking;
      bool connected;
      bool stored;
      int status;
      continue_func_t continueFunc;

    public:
      DB(st_mysql *db = 0);
      ~DB();

      st_mysql *getDB() const {return db;}

      // Options
      void setInitCommand(const std::string &cmd);
      void enableCompression();
      void setConnectTimeout(unsigned secs);
      void setLocalInFile(bool enable);
      void enableNamedPipe();
      void setProtocol(protocol_t protocol);
      void setReconnect(bool enable);
      void setReadTimeout(unsigned secs);
      void setWriteTimeout(unsigned secs);
      void setDefaultFile(const std::string &path);
      void readDefaultGroup(const std::string &path);
      void setReportDataTruncation(bool enable);
      void setCharacterSet(const std::string &name);
      void enableNonBlocking();

      // Connection
      void connect(const std::string &host = "localhost",
                   const std::string &user = "root",
                   const std::string &password = std::string(),
                   const std::string &dbName = std::string(),
                   unsigned port = 3306,
                   const std::string &socketName = std::string(),
                   flags_t flags = FLAG_DEFAULTS);
      bool connectNB(const std::string &host = "localhost",
                     const std::string &user = "root",
                     const std::string &password = std::string(),
                     const std::string &dbName = std::string(),
                     unsigned port = 3306,
                     const std::string &socketName = std::string(),
                     flags_t flags = FLAG_DEFAULTS);
      void close();
      bool closeNB();
      void use(const std::string &db);
      bool useNB(const std::string &db);

      // Query
      void query(const std::string &s);
      bool queryNB(const std::string &s);

      // Result set
      void useResult();
      void storeResult();
      bool storeResultNB();
      bool haveResult() const;
      bool nextResult();
      bool nextResultNB();
      bool moreResults() const;
      void freeResult();
      bool freeResultNB();
      uint64_t getRowCount() const;
      uint64_t getAffectedRowCount() const;
      unsigned getFieldCount() const;
      bool fetchRow();
      bool fetchRowNB();
      bool haveRow() const;
      void seekRow(uint64_t row);
      void appendRow(JSON::Sync &sync, int first = 0, int last = -1) const;
      void insertRow(JSON::Sync &sync, int first = 0, int last = -1,
                     bool withNulls = true) const;
      void writeRowList(JSON::Sync &sync, int first = 0, int last = -1) const;
      void writeRowDict(JSON::Sync &sync, int first = 0, int last = -1,
                        bool withNulls = true) const;

      // Field
      Field getField(unsigned i) const;
      Field::type_t getType(unsigned i) const;
      unsigned getLength(unsigned i) const;
      const char *getData(unsigned i) const;
      void writeField(JSON::Sync &sync, unsigned i) const;

      // Field type
      bool isNull(unsigned i) const {return getField(i).isNull();}
      bool isInteger(unsigned i) const {return getField(i).isInteger();}
      bool isReal(unsigned i) const {return getField(i).isReal();}
      bool isNumber(unsigned i) const {return getField(i).isNumber();}
      bool isBit(unsigned i) const {return getField(i).isBit();}
      bool isTime(unsigned i) const {return getField(i).isTime();}
      bool isBlob(unsigned i) const {return getField(i).isBlob();}
      bool isString(unsigned i) const {return getField(i).isString();}
      bool isGeometry(unsigned i) const {return getField(i).isGeometry();}

      // Field getters
      bool getNull(unsigned i) const;
      std::string getString(unsigned i) const;
      bool getBoolean(unsigned i) const;
      double getDouble(unsigned i) const;
      uint32_t getU32(unsigned i) const;
      int32_t getS32(unsigned i) const;
      uint64_t getU64(unsigned i) const;
      int64_t getS64(unsigned i) const;
      uint64_t getBit(unsigned i) const;
      void getSet(unsigned i, std::set<std::string> &s) const;
      double getTime(unsigned i) const;
      std::string rowToString() const;

      // Error handling
      std::string getInfo() const;
      const char *getSQLState() const;
      bool hasError() const;
      std::string getError() const;
      unsigned getErrorNumber() const;
      void raiseError(const std::string &msg) const;
      unsigned getWarningCount() const;

      // Assertions
      void assertConnected() const;
      void assertPending() const;
      void assertNotPending() const;
      void assertNonBlocking() const;
      void assertHaveResult() const;
      void assertHaveRow() const;
      void assertNotHaveResult() const;
      void assertInFieldRange(unsigned i) const;

      // Non-blocking API
      bool isNonBlocking() const {return nonBlocking;}
      bool isPending() const {return status;}
      bool continueNB(unsigned ready);
      bool waitRead() const;
      bool waitWrite() const;
      bool waitExcept() const;
      bool waitTimeout() const;
      int getSocket() const;
      double getTimeout() const;

      // Formatting
      std::string escape(const std::string &s) const;
      static std::string toHex(const std::string &s);
      std::string format(const std::string &s, const JSON::Dict &dict) const;

      // Library
      static void libraryInit(int argc = 0, char *argv[] = 0,
                              char *groups[] = 0);
      static void libraryEnd();
      static const char *getClientInfo();

      // Thread API
      static void threadInit();
      static void threadEnd();
      static bool threadSafe();

    protected:
      // Continue non-blocking calls
      bool closeContinue(unsigned ready);
      bool connectContinue(unsigned ready);
      bool useContinue(unsigned ready);
      bool queryContinue(unsigned ready);
      bool storeResultContinue(unsigned ready);
      bool nextResultContinue(unsigned ready);
      bool freeResultContinue(unsigned ready);
      bool fetchRowContinue(unsigned ready);
    };
  }
}

#endif // CB_MARIADB_DB_H

