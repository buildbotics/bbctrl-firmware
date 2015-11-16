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

#include "LevelDB.h"

#include <cbang/Exception.h>

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/iterator.h>
#include <leveldb/write_batch.h>

#undef CBANG_EXCEPTION
#define CBANG_EXCEPTION CBANG_EXCEPTION_SUBCLASS(LevelDBException)

using namespace cb;
using namespace std;

namespace {
  const string nsLast =
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
}


string LevelDBNS::nsKey(const string &key) const {
  return name + key;
}


string LevelDBNS::stripKey(const string &key) const {
  if (key.length() < name.length())
    THROWS("Invalid key '" << key << "' for namespace '" << name << "'");
  return key.substr(name.length());
}


bool LevelDBNS::inNS(const string &key) const {
  return name.empty() || key.compare(0, name.length(), name) == 0;
}


void LevelDBNS::check(const leveldb::Status &s, const std::string &key) const {
  if (s.ok()) return;
  if (s.IsNotFound()) THROWS("DB ERROR: Not '" << nsKey(key) << "' found");
  THROWS("DB ERROR: " << s.ToString());
}


int LevelDB::Comparator::operator()(const string &, const string &) const {
  THROW("Must implement one of the compare functions");
}


bool LevelDB::Iterator::valid() const {
  return it->Valid() && inNS(it->key().ToString());
}


void LevelDB::Iterator::first() {
  if (hasNS()) it->Seek(getNS());
  else it->SeekToFirst();
}


void LevelDB::Iterator::last() {
  if (hasNS()) {
    seek(nsLast);
    if (it->Valid()) it->Prev();

  } else it->SeekToLast();
}


void LevelDB::Iterator::seek(const string &key) {
  it->Seek(nsKey(key));
}


void LevelDB::Iterator::next() {
  if (!valid()) THROW("Cannot call next() on invalid iterator");
  it->Next();
}


void LevelDB::Iterator::prev() {
  if (!valid()) THROW("Cannot call prev() on invalid iterator");
  it->Prev();
}


string LevelDB::Iterator::key() const {
  if (!valid()) THROW("Cannot call key() on invalid iterator");
  return stripKey(it->key().ToString());
}


string LevelDB::Iterator::value() const {
  if (!valid()) THROW("Cannot call value() on invalid iterator");
  return it->value().ToString();
}


bool LevelDB::Iterator::operator==(const Iterator &it) const {
  if (!valid() && !it.valid()) return true;
  if (!valid() || !it.valid()) return false;
  return this->it->key().ToString() == it.it->key().ToString();
}


LevelDB::Batch::Batch(const SmartPointer<leveldb::DB> &db,
                      const SmartPointer<leveldb::WriteBatch> &batch,
                      const string &name) :
  LevelDBNS(name), db(db), batch(batch) {}


LevelDB::Batch::~Batch() {}


LevelDB::Batch LevelDB::Batch::ns(const std::string &name) {
  return Batch(db, batch, getNS() + name);
}


void LevelDB::Batch::clear() {
  batch->Clear();
}


void LevelDB::Batch::set(const string &key, const string &value) {
  batch->Put(nsKey(key), value);
}


void LevelDB::Batch::erase(const string &key) {
  batch->Delete(nsKey(key));
}


void LevelDB::Batch::eraseAll(int options) {
  Iterator it(db->NewIterator(getReadOptions(options)), getNS());
  for (it.first(); it.valid(); it++) erase(it.key());
}


void LevelDB::Batch::commit(int options) {
  check(db->Write(getWriteOptions(options), batch.get()));
}


namespace {
  class LevelDBComparator : public leveldb::Comparator {
  public:
    SmartPointer<LevelDB::Comparator> comparator;

    LevelDBComparator(const SmartPointer<LevelDB::Comparator> &comparator) :
      comparator(comparator) {}

    int Compare(const leveldb::Slice &a, const leveldb::Slice &b) const {
      return (*comparator)(a.data(), a.size(), b.data(), b.size());
    }

    const char *Name() const {return comparator->getName().c_str();}
    void FindShortestSeparator(std::string *, const leveldb::Slice &) const {}
    void FindShortSuccessor(std::string *) const {}
  };
}


LevelDB::LevelDB(const SmartPointer<Comparator> &comparator) {
  if (!comparator.isNull())
    this->comparator = new LevelDBComparator(comparator);
}


LevelDB::LevelDB(const string &name) : LevelDBNS(name) {}


LevelDB::LevelDB(const string &name,
                 const SmartPointer<Comparator> &comparator) :
  LevelDBNS(name) {
  if (!comparator.isNull())
    this->comparator = new LevelDBComparator(comparator);
}


LevelDB::~LevelDB() {}


LevelDB LevelDB::ns(const string &name) {
  LevelDB db(getNS() + name);
  db.db = this->db;
  db.comparator = comparator;
  return db;
}


void LevelDB::open(const string &path, int options) {
  leveldb::Options opts = getOptions(options);
  if (!comparator.isNull()) opts.comparator = comparator.get();

  leveldb::DB *db;
  leveldb::Status status = leveldb::DB::Open(opts, path, &db);
  if (!status.ok())
    THROWS("Failed to open DB '" << path << "': " << status.ToString());

  this->db = db;
}


void LevelDB::close() {
  db.release();
}


bool LevelDB::has(const string &key, int options) const {
  string value;
  leveldb::Status s = db->Get(getReadOptions(options), nsKey(key), &value);
  if (s.IsNotFound()) return false;
  check(s, key);
  return true;
}


string LevelDB::get(const string &key, int options) const {
  string value;
  leveldb::Status s = db->Get(getReadOptions(options), nsKey(key), &value);
  check(s, key);
  return value;
}


string LevelDB::get(const string &key, const string &defaultValue,
                    int options) const {
  string value;
  leveldb::Status s = db->Get(getReadOptions(options), nsKey(key), &value);
  if (s.IsNotFound()) return defaultValue;
  check(s, key);
  return value;
}


void LevelDB::set(const string &key, const string &value, int options) {
  check(db->Put(getWriteOptions(options), nsKey(key), value), key);
}


void LevelDB::erase(const string &key, int options) {
  check(db->Delete(getWriteOptions(options), nsKey(key)), key);
}


void LevelDB::eraseAll(int options) {
  for (Iterator it = begin(); it.valid(); it++)
    erase(it.key(), options);
}


LevelDB::Iterator LevelDB::iterator(int options) const {
  return Iterator(db->NewIterator(getReadOptions(options)), getNS());
}


LevelDB::Iterator LevelDB::begin(int options) const {
  Iterator it = iterator(options);
  it.first();
  return it;
}


LevelDB::Iterator LevelDB::end(int options) const {
  Iterator it = iterator(options);
  it.last();
  it.next();
  return it;
}


LevelDB::Batch LevelDB::batch() {
  return Batch(db, new leveldb::WriteBatch, getNS());
}


string LevelDB::getProperty(const string &name) {
  string value;
  if (!db->GetProperty(name, &value))
    THROWS("Invalid property '" << name << "'");
  return value;
}


void LevelDB::compact(const string &begin, const string &end) {
  SmartPointer<leveldb::Slice> beginSlice =
    begin.empty() ? 0 : new leveldb::Slice(begin);
  SmartPointer<leveldb::Slice> endSlice =
    end.empty() ? 0 : new leveldb::Slice(begin);

  db->CompactRange(beginSlice.get(), endSlice.get());
}


leveldb::Options LevelDB::getOptions(int options) {
  // TODO Support other options

  leveldb::Options opts;

  opts.create_if_missing = options & CREATE_IF_MISSING;
  opts.error_if_exists = options & ERROR_IF_EXISTS;
  opts.paranoid_checks = options & PARANOID_CHECKS;
  if (options & NO_COMPRESSION) opts.compression = leveldb::kNoCompression;

  return opts;
}


leveldb::ReadOptions LevelDB::getReadOptions(int options) {
  leveldb::ReadOptions opts;

  opts.verify_checksums = options & VERIFY_CHECKSUMS;
  opts.fill_cache = options & FILL_CACHE;

  return opts;
}


leveldb::WriteOptions LevelDB::getWriteOptions(int options) {
  leveldb::WriteOptions opts;

  opts.sync = options & SYNC;

  return opts;
}

#endif // HAVE_LEVELDB
