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

#ifdef HAVE_LEVELDB
#ifndef CB_LEVEL_DB_H
#define CB_LEVEL_DB_H

#include <cbang/SmartPointer.h>

#include <string>

namespace leveldb {
  class DB;
  class Comparator;
  class Iterator;
  class WriteBatch;
  class Status;
  class Options;
  class ReadOptions;
  class WriteOptions;
}


namespace cb {
  class LevelDBNS {
    std::string name;

  public:
    LevelDBNS(const std::string &name = std::string()) : name(name) {}

    bool hasNS() const {return !name.empty();}
    const std::string &getNS() const {return name;}

    std::string nsKey(const std::string &key) const;
    std::string stripKey(const std::string &key) const;
    bool inNS(const std::string &key) const;
    void check(const leveldb::Status &s,
               const std::string &key = std::string()) const;
  };


  class LevelDB : public LevelDBNS {
    SmartPointer<leveldb::DB> db;
    SmartPointer<leveldb::Comparator> comparator;

  public:
    typedef enum {
      CREATE_IF_MISSING = 1 << 0,
      ERROR_IF_EXISTS   = 1 << 1,
      PARANOID_CHECKS   = 1 << 2,
      NO_COMPRESSION    = 1 << 3,
      VERIFY_CHECKSUMS  = 1 << 4,
      FILL_CACHE        = 1 << 5,
      SYNC              = 1 << 6,
    } options_t;


    class Comparator {
      std::string name;

    public:
      Comparator(const std::string &name) : name(name) {}
      virtual ~Comparator() {}

      const std::string &getName() const {return name;}

      virtual int operator()(const std::string &key1,
                             const std::string &key2) const {
        CBANG_THROW("Must implement one of the compare functions");
      }

      virtual int operator()(const char *key1, unsigned len1,
                             const char *key2, unsigned len2) const {
        return (*this)(std::string(key1, len1), std::string(key2, len2));
      }
    };


    class Iterator : public LevelDBNS {
      SmartPointer<leveldb::Iterator> it;

    public:
      Iterator(const SmartPointer<leveldb::Iterator> &it,
               const std::string &name) :
        LevelDBNS(name), it(it) {}

      bool valid() const;
      void first();
      void last();
      void seek(const std::string &key);
      void next();
      void prev();
      std::string key() const;
      std::string value() const;

      void operator++() {next();}
      void operator++(int) {next();}
      bool operator==(const Iterator &it) const;
      bool operator!=(const Iterator &it) const {return !(*this == it);}
    };


    class Batch : public LevelDBNS {
      SmartPointer<leveldb::DB> db;
      SmartPointer<leveldb::WriteBatch> batch;

    public:
      Batch(const SmartPointer<leveldb::DB> &db,
            const SmartPointer<leveldb::WriteBatch> &batch,
            const std::string &name);
      ~Batch();

      Batch ns(const std::string &name);

      void clear();
      void set(const std::string &key, const std::string &value);
      void erase(const std::string &key);
      void eraseAll(int options = 0);
      void commit(int options = 0);
    };


    LevelDB(const SmartPointer<Comparator> &comparator = 0);
    LevelDB(const std::string &name);
    LevelDB(const std::string &name,
            const SmartPointer<Comparator> &comparator);

    ~LevelDB();

    leveldb::DB &getDB() {return *db;}

    LevelDB ns(const std::string &name);

    void open(const std::string &path, int options = 0);
    void close();

    bool has(const std::string &key, int options = 0) const;
    std::string get(const std::string &key, int options = 0) const;
    std::string get(const std::string &key,
                    const std::string &defaultValue, int options = 0) const;
    void set(const std::string &key, const std::string &value, int options = 0);
    void erase(const std::string &key, int options = 0);
    void eraseAll(int options = 0);

    Iterator iterator(int options = 0) const;
    Iterator begin(int options = 0) const;
    Iterator end(int options = 0) const;
    Batch batch();

    std::string getProperty(const std::string &name);
    void compact(const std::string &begin = std::string(),
                 const std::string &end = std::string());

    static leveldb::Options getOptions(int options);
    static leveldb::ReadOptions getReadOptions(int options);
    static leveldb::WriteOptions getWriteOptions(int options);
  };
}

#endif // CB_LEVEL_DB_H
#endif // HAVE_LEVELDB
