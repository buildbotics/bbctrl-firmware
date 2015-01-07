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

#ifndef CB_CERTIFICATE_H
#define CB_CERTIFICATE_H

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>

#include <iostream>

typedef struct x509_st X509;


namespace cb {
  class KeyPair;
  class CertificateContext;

  class Certificate {
    X509 *cert;

  public:
    Certificate(const Certificate &o);
    Certificate(X509 *cert = 0);
    Certificate(const std::string &pem);
    ~Certificate();

    Certificate &operator=(const Certificate &o);

    X509 *getX509() const {return cert;}

    void getPublicKey(KeyPair &key) const;
    SmartPointer<KeyPair> getPublicKey() const;
    void setPublicKey(const KeyPair &key);

    void setVersion(int version);
    int getVersion() const;

    void setSerial(long serial);
    long getSerial() const;

    void setNotBefore(uint64_t x = 0);
    void setNotAfter(uint64_t x);

    void setIssuer(const Certificate &issuer);

    void addNameEntry(const std::string &name, const std::string &value);
    std::string getNameEntry(const std::string &name) const;

    bool hasExtension(const std::string &name);
    std::string getExtension(const std::string &name);
    void addExtension(const std::string &name, const std::string &value,
                      CertificateContext *ctx = 0);
    static void addExtensionAlias(const std::string &alias,
                                  const std::string &name);

    bool hasAuthorityKeyIdentifer() const;
    bool issued(const Certificate &o) const;

    void sign(KeyPair &key, const std::string &digest = "sha256");
    void verify();

    std::string toString() const;
    void set(const std::string &pem);

    std::istream &read(std::istream &stream);
    std::ostream &write(std::ostream &stream) const;
  };


  inline std::istream &operator>>(std::istream &stream, Certificate &cert) {
    return cert.read(stream);
  }

  inline std::ostream &operator<<(std::ostream &stream,
                                  const Certificate &cert) {
    return cert.write(stream);
  }
}

#endif // CB_CERTIFICATE_H

