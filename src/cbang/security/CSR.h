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

#ifndef CB_CSR_H
#define CB_CSR_H

#include <cbang/SmartPointer.h>

#include <iostream>
#include <string>

typedef struct X509_req_st X509_REQ;


namespace cb {
  class KeyPair;

  class CSR {
    X509_REQ *csr;

    struct private_t;
    private_t *pri;

  public:
    CSR();
    CSR(const std::string &pem);
    ~CSR();

    X509_REQ *getX509_REQ() const {return csr;}

    void getPublicKey(KeyPair &key);
    SmartPointer<KeyPair> getPublicKey();

    void addNameEntry(const std::string &name, const std::string &value);
    std::string getNameEntry(const std::string &name) const;

    bool hasExtension(const std::string &name);
    std::string getExtension(const std::string &name);
    void addExtension(const std::string &name, const std::string &value);

    void sign(const KeyPair &key, const std::string &digest = "sha256");
    void verify();

    std::string toString() const;

    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream) const;
    std::ostream &print(std::ostream &stream) const;
  };

  inline std::istream &operator>>(std::istream &stream, CSR &csr) {
    return csr.read(stream);
  }

  inline std::ostream &operator<<(std::ostream &stream, const CSR &csr) {
    return csr.write(stream);
  }
}

#endif // CB_CSR_H

