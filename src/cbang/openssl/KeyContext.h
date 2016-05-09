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

#ifndef CB_KEY_CONTEXT_H
#define CB_KEY_CONTEXT_H

#include <cbang/StdTypes.h>

#include <string>


typedef struct evp_pkey_ctx_st EVP_PKEY_CTX;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct engine_st ENGINE;


namespace cb {
  class KeyPair;
  class KeyGenCallback;

  class KeyContext {
    EVP_PKEY_CTX *ctx;
    bool deallocate;

  public:
    KeyContext(int nid, ENGINE *e = 0);
    KeyContext(const std::string &algorithm, ENGINE *e = 0);
    KeyContext(const KeyPair &keyPair);
    KeyContext(EVP_PKEY *pkey = 0, ENGINE *e = 0);
    KeyContext(const KeyContext &kc);
    KeyContext(EVP_PKEY_CTX *ctx, bool deallocate);
    ~KeyContext();

    EVP_PKEY_CTX *getEVP_PKEY_CTX() const {return ctx;}

    typedef enum {
      NO_PADDING = 100000,
      PKCS1_PADDING,
      SSLV23_PADDING,
      PKCS1_OAEP_PADDING,
      X931_PADDING,
    } padding_t;

    // Set options
    void setRSAPadding(padding_t padding);
    void setRSAPSSSaltLen(int len);
    void setRSABits(int bits);
    void setRSAPubExp(uint64_t exp);
    void setDSABits(int bits);
    void setDHPrimeLen(int len);
    void setDHGenerator(int gen);
    void setECCurve(const std::string &curve);
    void setKeyGenCallback(KeyGenCallback *callback);
    void setSignatureMD(const std::string &digest);

    // Key generation
    void keyGenInit();
    void keyGen(KeyPair &key);

    // Parameter generation
    void paramGenInit();
    void paramGen(KeyPair &key);

    // Sign
    void signInit();
    size_t sign(uint8_t *sigData, size_t sigLen,
                const uint8_t *msgData, size_t msgLen);
    std::string sign(const std::string &msg);

    // Verify signature
    void verifyInit();
    void verify(const uint8_t *sigData, size_t sigLen,
                const uint8_t *msgData, size_t msgLen);
    void verify(const std::string &sig, const std::string &msg);

    // Recover signature data
    void verifyRecoverInit();
    size_t verifyRecover(uint8_t *msgData, size_t msgLen,
                         const uint8_t *sigData, size_t sigLen);
    std::string verifyRecover(const std::string &sig);

    // Encrypt
    void encryptInit();
    size_t encrypt(uint8_t *outData, size_t outLen,
                   const uint8_t *inData, size_t inLen);
    std::string encrypt(const std::string &in);

    // Decrypt
    void decryptInit();
    size_t decrypt(uint8_t *outData, size_t outLen,
                   const uint8_t *inData, size_t inLen);
    std::string decrypt(const std::string &in);

    // Derive shared secret (DH EC)
    void deriveInit();
    void setDerivePeer(const KeyPair &key);
    size_t derive(uint8_t *keyData, size_t keyLen);
    std::string derive();
  };
}

#endif // CB_KEY_CONTEXT_H
