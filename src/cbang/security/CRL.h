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

#ifndef CB_CRL_H
#define CB_CRL_H

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>

#include <iostream>

typedef struct X509_crl_st X509_CRL;


namespace cb {
  class KeyPair;
  class Certificate;

  class CRL {
    X509_CRL *crl;

  public:
    CRL(X509_CRL *crl = 0);
    CRL(const std::string &pem);
    ~CRL();

    X509_CRL *getX509_CRL() const {return crl;}

    void setVersion(int version);
    int getVersion() const;

    void setIssuer(const Certificate &cert);

    void setLastUpdate(uint64_t x);
    void setNextUpdate(uint64_t x);

    bool hasExtension(const std::string &name);
    std::string getExtension(const std::string &name);
    void addExtension(const std::string &name, const std::string &value);

    void revoke(const Certificate &cert,
                const std::string &reason = "unspecified", uint64_t ts = 0);
    bool wasRevoked(const Certificate &cert) const;

    void sort();
    void sign(KeyPair &key, const std::string &digest = "sha256");
    void verify(KeyPair &key);

    std::string toString() const;

    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream) const;
  };


  inline std::istream &operator>>(std::istream &stream, CRL &crl) {
    return crl.read(stream);
  }

  inline std::ostream &operator<<(std::ostream &stream,
                                  const CRL &crl) {
    return crl.write(stream);
  }
}

#endif // CB_CRL_H

