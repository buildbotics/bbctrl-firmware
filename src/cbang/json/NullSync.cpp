#include "NullSync.h"

using namespace cb::JSON;


bool NullSync::inList() const {
  return !stack.empty() && stack.back() == ValueType::JSON_LIST;
}


bool NullSync::inDict() const {
  return !stack.empty() && stack.back() == ValueType::JSON_DICT;
}


void NullSync::end() {
  if (inList()) endList();
  else if (inDict()) endDict();
  else THROW("Not in list or dict");
}


void NullSync::close() {
  assertWriteNotPending();
  if (!stack.empty()) THROWS("Writer closed with open " << stack.back());
}


void NullSync::reset() {
  stack.clear();
  keyStack.clear();
  canWrite = true;
}


void NullSync::writeNull() {
  assertCanWrite();
}


void NullSync::writeBoolean(bool value) {
  assertCanWrite();
}


void NullSync::write(double value) {
  assertCanWrite();
}


void NullSync::write(const std::string &value) {
  assertCanWrite();
}


void NullSync::beginList(bool simple) {
  assertCanWrite();
  stack.push_back(ValueType::JSON_LIST);
  canWrite = false;
}


void NullSync::beginAppend() {
  assertWriteNotPending();
  if (!inList()) THROW("Not a List");
  canWrite = true;
}


void NullSync::endList() {
  assertWriteNotPending();
  if (!inList()) THROW("Not a List");

  stack.pop_back();
}


void NullSync::beginDict(bool simple) {
  assertCanWrite();
  stack.push_back(ValueType::JSON_DICT);
  keyStack.push_back(keys_t());
  canWrite = false;
}


bool NullSync::has(const std::string &key) const {
  if (!inDict()) THROW("Not a Dict");
  return keyStack.back().find(key) != keyStack.back().end();
}


void NullSync::beginInsert(const std::string &key) {
  assertWriteNotPending();
  if (has(key)) THROWS("Key '" << key << "' already written to output");
  keyStack.back().insert(key);
  canWrite = true;
}


void NullSync::endDict() {
  assertWriteNotPending();
  if (!inDict()) THROW("Not a Dict");

  stack.pop_back();
  keyStack.pop_back();
}


void NullSync::assertCanWrite() {
  if (!canWrite) THROW("Not ready for write");
  canWrite = false;
}


void NullSync::assertWriteNotPending() {
  if (canWrite) THROW("Expected write");
}
