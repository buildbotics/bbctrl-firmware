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

#ifndef CBANG_KEY_PAIR_H
#define CBANG_KEY_PAIR_H

#include "KeyGenCallback.h"

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>

#include <iostream>
#include <string>

typedef struct evp_pkey_st EVP_PKEY;


namespace cb {
  class KeyPair {
    EVP_PKEY *key;

  public:
    KeyPair(EVP_PKEY *key) : key(key) {}
    KeyPair();
    ~KeyPair();

    EVP_PKEY *getEVP_PKEY() const {return key;}
    void setEVP_PKEY(EVP_PKEY *key) {this->key = key;}

    bool isValid() const;
    bool isRSA() const;
    bool isDSA() const;
    bool isDH() const;
    bool isEC() const;

    bool hasPublic() const;
    bool hasPrivate() const;

    unsigned size() const;
    bool match(const KeyPair &o) const;

    // Generate
    void generateRSA(unsigned bits = 4096, uint64_t pubExp = 65537,
                     SmartPointer<KeyGenCallback> callback = 0);
    void generateDSA(unsigned bits = 4096,
                     SmartPointer<KeyGenCallback> callback = 0);
    void generateDH(unsigned primeLen, int generator = 2,
                    SmartPointer<KeyGenCallback> callback = 0);
    void generateEC(const std::string &curve = "secp192k1",
                    SmartPointer<KeyGenCallback> callback = 0);

    // To PEM string
    std::string publicToString() const;
    std::string privateToString() const;
    std::string toString() const;

    // Read PEM
    std::istream &readPublic(std::istream &stream);
    void readPublic(const std::string &pem);
    std::istream &readPrivate(std::istream &stream);
    void readPrivate(const std::string &pem);
    std::istream &read(std::istream &stream);

    // Write PEM
    std::ostream &writePublic(std::ostream &stream) const;
    std::ostream &writePrivate(std::ostream &stream) const;
    std::ostream &write(std::ostream &stream) const;

    // Print ASN1
    std::ostream &printPublic(std::ostream &stream, int indent = 0) const;
    std::ostream &printPrivate(std::ostream &stream, int indent = 0) const;
    std::ostream &print(std::ostream &stream, int indent = 0) const;
  };


  inline std::istream &operator>>(std::istream &stream, KeyPair &key) {
    return key.read(stream);
  }

  inline std::ostream &operator<<(std::ostream &stream, const KeyPair &key) {
    return key.write(stream);
  }
}

#endif // CBANG_KEY_PAIR_H

