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

#define NOCRYPT // For WIN32 to avoid conflicts with openssl

#include "KeyPair.h"

#include "SSL.h"
#include "KeyContext.h"
#include "BIStream.h"
#include "BOStream.h"

#include <cbang/Exception.h>
#include <cbang/SmartPointer.h>
#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>

#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/dsa.h>
#include <openssl/ec.h>
#include <openssl/engine.h>

#include <string.h>

#include <sstream>

using namespace cb;
using namespace std;


namespace {
  int password_cb(char *buf, int size, int rwflag, void *cb) {
    try {
      string pw = (*(PasswordCallback *)cb)(rwflag, (unsigned)size);

      int length = min(size, (int)pw.length());
      memcpy(buf, pw.c_str(), length);
      return length;

    } CATCH_ERROR;

    return 0;
  }
}


KeyPair::KeyPair(const KeyPair &o) : key(o.key) {
  CRYPTO_add(&(key->references), 1, CRYPTO_LOCK_EVP_PKEY);
}


KeyPair::KeyPair() {
  SSL::init();
  key = EVP_PKEY_new();
}


KeyPair::KeyPair(const string &key, mac_key_t type, ENGINE *e) {
  SSL::init();
  this->key =
    EVP_PKEY_new_mac_key(type == HMAC_KEY ? EVP_PKEY_HMAC : EVP_PKEY_CMAC,
                         e, (uint8_t *)key.c_str(), key.length());

  if (!this->key) THROWS("Failed to create MAC key: " << SSL::getErrorStr());
}


KeyPair::~KeyPair() {
  if (key) EVP_PKEY_free(key);
}


bool KeyPair::isValid() const {
  return key->pkey.ptr;
}


bool KeyPair::isRSA() const {
  return EVP_PKEY_type(key->type) == EVP_PKEY_RSA;
}


bool KeyPair::isDSA() const {
  return EVP_PKEY_type(key->type) == EVP_PKEY_DSA;
}


bool KeyPair::isDH() const {
  return EVP_PKEY_type(key->type) == EVP_PKEY_DH;
}


bool KeyPair::isEC() const {
  return EVP_PKEY_type(key->type) == EVP_PKEY_EC;
}


bool KeyPair::hasPublic() const {
  switch (EVP_PKEY_type(key->type)) {
  case EVP_PKEY_RSA: return key->pkey.rsa->e;
  case EVP_PKEY_DSA: return key->pkey.dsa->pub_key;
  case EVP_PKEY_DH: return key->pkey.dh->pub_key;
  case EVP_PKEY_EC: return EC_KEY_get0_public_key(key->pkey.ec);
  default: THROW("Invalid key type");
  }
}


bool KeyPair::hasPrivate() const {
  switch (EVP_PKEY_type(key->type)) {
  case EVP_PKEY_RSA: return key->pkey.rsa->d;
  case EVP_PKEY_DSA: return key->pkey.dsa->priv_key;
  case EVP_PKEY_DH: return key->pkey.dh->priv_key;
  case EVP_PKEY_EC: return EC_KEY_get0_private_key(key->pkey.ec);
  default: THROW("Invalid key type");
  }
}


unsigned KeyPair::size() const {
  return EVP_PKEY_size(key);
}


bool KeyPair::match(const KeyPair &o) const {
  switch (EVP_PKEY_cmp(key, o.key)) {
  case 0: return false;
  case 1: return true;
  default: THROWS("Error comparing keys: " << SSL::getErrorStr());
  }
}


void KeyPair::generateRSA(unsigned bits, uint64_t pubExp,
                          SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_RSA);
  ctx.keyGenInit();
  ctx.setRSABits(bits);
  ctx.setRSAPubExp(pubExp);
  ctx.setKeyGenCallback(callback.get());
  ctx.keyGen(*this);
}


void KeyPair::generateDSA(unsigned bits,
                          SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_DSA);
  ctx.keyGenInit();
  ctx.setDSABits(bits);
  ctx.setKeyGenCallback(callback.get());
  ctx.keyGen(*this);
}


void KeyPair::generateDH(unsigned primeLen, int generator,
                         SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_DH);
  ctx.keyGenInit();
  ctx.setDHPrimeLen(primeLen);
  ctx.setDHGenerator(generator);
  ctx.setKeyGenCallback(callback.get());
  ctx.keyGen(*this);
}


void KeyPair::generateEC(const string &curve,
                         SmartPointer<KeyGenCallback> callback) {
  KeyContext ctx(EVP_PKEY_DH);
  ctx.keyGenInit();
  ctx.setECCurve(curve);
  ctx.setKeyGenCallback(callback.get());
  ctx.keyGen(*this);
}


string KeyPair::publicToString() const {
  ostringstream str;
  writePublic(str);
  return str.str();
}


string KeyPair::privateToString() const {
  ostringstream str;
  writePrivate(str);
  return str.str();
}


string KeyPair::toString() const {
  ostringstream str;
  write(str);
  return str.str();
}


istream &KeyPair::readPublic(istream &stream) {
  BIStream bio(stream);

  if (!PEM_read_bio_PUBKEY(bio.getBIO(), &key, 0, 0))
    THROWS("Failed to read public key: " << SSL::getErrorStr());

  return stream;
}


void KeyPair::readPublic(const string &pem) {
  istringstream str(pem);
  readPublic(str);
}


istream &KeyPair::readPrivate(istream &stream,
                              SmartPointer<PasswordCallback> callback) {
  BIStream bio(stream);

  if (!PEM_read_bio_PrivateKey
      (bio.getBIO(), &key, callback.isNull() ? 0 : password_cb, callback.get()))
    THROWS("Failed to read private key: " << SSL::getErrorStr());

  return stream;
}


void KeyPair::readPrivate(const string &pem,
                          SmartPointer<PasswordCallback> callback) {
  istringstream str(pem);
  readPrivate(str, callback);
}


istream &KeyPair::read(istream &stream,
                       SmartPointer<PasswordCallback> callback) {
  try {return readPrivate(stream, callback);} catch (...) {} // Ignore
  return readPublic(stream);
}


ostream &KeyPair::writePublic(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_PUBKEY(bio.getBIO(), key))
    THROWS("Failed to write public key: " << SSL::getErrorStr());

  return stream;
}


ostream &KeyPair::writePrivate(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_PrivateKey(bio.getBIO(), key, 0, 0, 0, 0, 0))
    THROWS("Failed to write private key: " << SSL::getErrorStr());

  return stream;
}


ostream &KeyPair::write(ostream &stream) const {
  if (hasPrivate()) return writePrivate(stream);
  return writePublic(stream);
}


ostream &KeyPair::printPublic(ostream &stream, int indent) const {
  BOStream bio(stream);

  if (!EVP_PKEY_print_public(bio.getBIO(), key, indent, 0))
    THROWS("Failed to print public key: " << SSL::getErrorStr());

  return stream;
}


ostream &KeyPair::printPrivate(ostream &stream, int indent) const {
  BOStream bio(stream);

  if (!EVP_PKEY_print_private(bio.getBIO(), key, indent, 0))
    THROWS("Failed to print private key: " << SSL::getErrorStr());

  return stream;
}


ostream &KeyPair::print(ostream &stream, int indent) const {
  if (hasPrivate()) return printPrivate(stream, indent);
  return printPublic(stream, indent);
}
