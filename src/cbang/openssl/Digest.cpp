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

#include "Digest.h"

#include "SSL.h"
#include "KeyPair.h"
#include "KeyContext.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/net/Base64.h>

#include <openssl/evp.h>

using namespace cb;
using namespace std;


Digest::Digest(const string &digest) : md(0), ctx(0), initialized(false) {
  SSL::init();

  ctx = EVP_MD_CTX_create();
  if (!ctx) THROWS("Failed to created digest context: " << SSL::getErrorStr());

  md = EVP_get_digestbyname(digest.c_str());
  if (!md) THROWS("Unrecognized digest '" << digest);
}


Digest::~Digest() {
  EVP_MD_CTX_destroy(ctx);
}


unsigned Digest::size() const {
  return EVP_MD_CTX_size(ctx);
}


void Digest::init(ENGINE *e) {
  if (initialized) THROW("Digest already initialized");

  e = (e || !ctx) ? e : ctx->engine;
  if (!EVP_DigestInit_ex(ctx, md, e))
    THROWS("Error initializing digest context: " << SSL::getErrorStr());

  initialized = true;
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
  if (!initialized) init();

  if (!EVP_DigestUpdate(ctx, data, length))
      THROWS("Error updating digest: " << SSL::getErrorStr());
}


void Digest::finalize() {
  if (digest.empty()) digest.resize(size());

  if (!EVP_DigestFinal_ex(ctx, &digest[0], 0))
    THROWS("Error finalizing digest: " << SSL::getErrorStr());
}


SmartPointer<KeyContext> Digest::signInit(const KeyPair &key, ENGINE *e) {
  if (initialized) THROW("Digest already initialized");

  EVP_PKEY_CTX *pctx = 0;

  e = (e || !ctx) ? e : ctx->engine;
  if (!EVP_DigestSignInit(ctx, &pctx, md, e, key.getEVP_PKEY()))
    THROWS("Error initializing digest sign context: " << SSL::getErrorStr());

  initialized = true;

  return (pctx ? new KeyContext(pctx, false) : 0);
}


size_t Digest::sign(uint8_t *sigData, size_t sigLen) {
  if (EVP_DigestSignFinal(ctx, sigData, &sigLen) <= 0)
    THROWS("Failed to "
           << (sigData ? "sign digest" : "compute signature length") << ": "
           << SSL::getErrorStr());

  return sigLen;
}


string Digest::sign() {
  // Determine signature length
  size_t sigLen = sign(0, 0);

  // Allocate space
  SmartPointer<uint8_t>::Array sigData = new uint8_t[sigLen];

  // Sign
  sign(sigData.get(), sigLen);

  // Return result
  return string((char *)sigData.get(), sigLen);
}


SmartPointer<KeyContext> Digest::verifyInit(const KeyPair &key, ENGINE *e) {
  if (initialized) THROW("Digest already initialized");

  EVP_PKEY_CTX *pctx = 0;

  e = (e || !ctx) ? e : ctx->engine;
  if (!EVP_DigestVerifyInit(ctx, &pctx, md, e, key.getEVP_PKEY()))
    THROWS("Error initializing digest verify context: " << SSL::getErrorStr());

  initialized = true;

  return (pctx ? new KeyContext(pctx, false) : 0);
}


bool Digest::verify(const uint8_t *sigData, size_t sigLen) {
  switch (EVP_DigestVerifyFinal(ctx, (uint8_t *)sigData, sigLen)) {
  case 0: return false;
  case 1: return true;
  default: THROWS("Error verifing digest signature: " << SSL::getErrorStr());
  }
}


bool Digest::verify(const string &sig) {
  return verify((const uint8_t *)sig.c_str(), sig.size());
}


string Digest::toString() const {
  if (digest.empty()) THROW("Digest not finalized");
  return string((const char *)&digest[0], size());
}


string Digest::toString() {
  if (digest.empty()) finalize();
  return const_cast<const Digest *>(this)->toString();
}


string Digest::toHexString() const {
  if (digest.empty()) THROW("Digest not finalized");

  string result;
  unsigned s = size();
  for (unsigned i = 0; i < s; i++)
    result += String::printf("%02x", digest.at(i));
  return result;
}


string Digest::toHexString() {
  if (digest.empty()) finalize();
  return const_cast<const Digest *>(this)->toHexString();
}


string Digest::toBase64(char pad, char a, char b) const {
  return Base64(pad, a, b).encode(toString());
}


string Digest::toBase64(char pad, char a, char b) {
  return Base64(pad, a, b).encode(toString());
}


unsigned Digest::getDigest(uint8_t *buffer, unsigned length) const {
  unsigned s = size();
  unsigned i;

  for (i = 0; (!length || i < length) && i < s; i++)
    buffer[i] = digest.at(i);

  return i;
}


unsigned Digest::getDigest(uint8_t *buffer, unsigned length) {
  if (digest.empty()) finalize();
  return const_cast<const Digest *>(this)->getDigest(buffer, length);
}


const vector<uint8_t> &Digest::getDigest() {
  if (digest.empty()) finalize();
  return const_cast<const Digest *>(this)->getDigest();
}


void Digest::reset() {
  digest.clear();
  initialized = false;
}


string Digest::hash(const string &s, const string &digest, ENGINE *e) {
  Digest d(digest);
  d.init(e);
  d.update(s);
  return d.toString();
}


string Digest::hashHex(const string &s, const string &digest, ENGINE *e) {
  Digest d(digest);
  d.init(e);
  d.update(s);
  return d.toHexString();
}


string Digest::sign(const KeyPair &key, const string &s, const string &digest,
                    ENGINE *e) {
  Digest d(digest);
  d.signInit(key, e);
  d.update(s);
  return d.sign();
}


bool Digest::verify(const KeyPair &key, const std::string &s, const string &sig,
                    const string &digest, ENGINE *e) {
  Digest d(digest);
  d.verifyInit(key, e);
  d.update(s);
  return d.verify(sig);
}


bool Digest::hasAlgorithm(const string &digest) {
  SSL::init();
  return EVP_get_digestbyname(digest.c_str());
}


string Digest::signHMAC(const string &key, const string &s,
                        const string &digest, ENGINE *e) {
  return sign(KeyPair(key, KeyPair::HMAC_KEY, e), s, digest, e);
}


bool Digest::verifyHMAC(const string &key, const string &s, const string &sig,
                        const string &digest, ENGINE *e) {
  return verify(KeyPair(key, KeyPair::HMAC_KEY, e), s, sig, digest, e);
}
