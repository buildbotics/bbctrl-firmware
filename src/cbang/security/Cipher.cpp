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

#include "Cipher.h"

#include "SSL.h"

#include <cbang/Exception.h>

#include <openssl/evp.h>

#include <string.h>

using namespace std;
using namespace cb;


Cipher::Cipher(const string &cipher, bool encrypt, const void *key,
               const void *iv, ENGINE *e) :
  ctx(new EVP_CIPHER_CTX), encrypt(encrypt) {
  SSL::init();
  OpenSSL_add_all_algorithms();

  EVP_CIPHER_CTX_init(ctx);

  const EVP_CIPHER *c = EVP_get_cipherbyname(cipher.c_str());
  if (!c) THROWS("Unrecognized cipher '" << cipher << "'");

  if (!(encrypt ? EVP_EncryptInit_ex : EVP_DecryptInit_ex)
      (ctx, c, e, (const unsigned char *)key, (const unsigned char *)iv))
    THROWS("Cipher init failed: " << SSL::getErrorStr());
}


Cipher::~Cipher() {
  if (ctx) {
    EVP_CIPHER_CTX_cleanup(ctx);
    EVP_CIPHER_CTX_free(ctx);
  }
}


unsigned Cipher::getKeyLength() const {
  return EVP_CIPHER_CTX_key_length(ctx);
}


void Cipher::setKey(const void *key) {
  if (!(encrypt ? EVP_EncryptInit_ex : EVP_DecryptInit_ex)
      (ctx, 0, 0, (const unsigned char *)key, 0))
    THROWS("Failed to set cipher key: " << SSL::getErrorStr());
}


unsigned Cipher::getIVLength() const {
  return EVP_CIPHER_CTX_iv_length(ctx);
}


void Cipher::setIV(const void *iv) {
  if (!(encrypt ? EVP_EncryptInit_ex : EVP_DecryptInit_ex)
      (ctx, 0, 0, 0, (const unsigned char *)iv))
    THROWS("Failed to set cipher IV: " << SSL::getErrorStr());
}


unsigned Cipher::getBlockSize() const {
  return EVP_CIPHER_CTX_block_size(ctx);
}


unsigned Cipher::update(void *out, unsigned length, const void *in,
                        unsigned inLen) {
  SmartPointer<char>::Array buf;
  if (out == in) {
    if (inLen < length)
      THROW("If using cipher in place then the intput buffer length must be "
            "at least as large as the output buffer");

    buf = new char[inLen];
    memcpy(buf.get(), in, inLen);
    in = buf.get();
  }

  if (!(encrypt ? EVP_EncryptUpdate : EVP_DecryptUpdate)
      (ctx, (unsigned char *)out, (int *)&length,
       (unsigned char *)in, inLen))
    THROWS("Error updating cipher: " << SSL::getErrorStr());

  return length;
}


unsigned Cipher::final(void *out, unsigned length) {
  if (!(encrypt ? EVP_EncryptFinal_ex : EVP_DecryptFinal_ex)
      (ctx, (unsigned char *)out, (int *)&length))
    THROWS("Error finalizing cipher: " << SSL::getErrorStr());

  return length;
}
