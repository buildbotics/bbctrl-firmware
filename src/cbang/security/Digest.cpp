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

#include "Digest.h"

#include "SSL.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <openssl/evp.h>

using namespace cb;
using namespace std;


Digest::Digest(const string &digest, ENGINE *e) : ctx(EVP_MD_CTX_create()) {
  SSL::init();

  const EVP_MD *md = EVP_get_digestbyname(digest.c_str());
  if (!md) THROWS("Unrecognized digest '" << digest);

  if (!EVP_DigestInit_ex(ctx, md, e))
    THROWS("Error initializing digest context: " << SSL::getErrorStr());
}


Digest::~Digest() {
  EVP_MD_CTX_destroy(ctx);
}


unsigned Digest::size() const {
  return EVP_MD_CTX_size(ctx);
}


void Digest::update(istream &stream) {
  uint8_t buffer[4096];

  do {
    stream.read((char *)buffer, 4096);
    update(buffer, stream.gcount());
  } while (stream);
}


void Digest::update(const string &data) {
  update((const uint8_t *)data.c_str(), data.length());
}


void Digest::update(const uint8_t *data, unsigned length) {
  if (!EVP_DigestUpdate(ctx, data, length))
      THROWS("Error updating digest: " << SSL::getErrorStr());
}


void Digest::finalize() {
  if (digest.isNull()) digest = new uint8_t[EVP_MAX_MD_SIZE];

  if (!EVP_DigestFinal_ex(ctx, digest.get(), 0))
    THROWS("Error finalizing digest: " << SSL::getErrorStr());
}


string Digest::toString() const {
  if (digest.isNull()) THROW("Digest not finalized");
  return string((const char *)digest.get(), size());
}


string Digest::toString() {
  if (digest.isNull()) finalize();
  return const_cast<const Digest *>(this)->toString();
}


string Digest::toHexString() const {
  if (digest.isNull()) THROW("Digest not finalized");

  string result;
  unsigned s = size();
  for (unsigned i = 0; i < s; i++) result += String::printf("%02x", digest[i]);
  return result;
}


string Digest::toHexString() {
  if (digest.isNull()) finalize();
  return const_cast<const Digest *>(this)->toHexString();
}


unsigned Digest::getDigest(uint8_t *buffer, unsigned length) const {
  unsigned s = size();
  unsigned i;

  for (i = 0; (!length || i < length) && i < s; i++)
    buffer[i] = digest[i];

  return i;
}


unsigned Digest::getDigest(uint8_t *buffer, unsigned length) {
  if (digest.isNull()) finalize();
  return const_cast<const Digest *>(this)->getDigest(buffer, length);
}


void Digest::reset() {
  digest.release();
  const EVP_MD *md = EVP_MD_CTX_md(ctx);
  ENGINE *e = ctx->engine;

  if (!EVP_DigestInit_ex(ctx, md, e))
    THROWS("Error initializing digest context: " << SSL::getErrorStr());
}


string Digest::hash(const string &s, const string &digest, ENGINE *e) {
  Digest d(digest, e);
  d.update(s);
  return d.toString();
}


string Digest::hashHex(const string &s, const string &digest, ENGINE *e) {
  Digest d(digest, e);
  d.update(s);
  return d.toHexString();
}


bool Digest::hasAlgorithm(const string &digest) {
  SSL::init();
  return EVP_get_digestbyname(digest.c_str());
}
