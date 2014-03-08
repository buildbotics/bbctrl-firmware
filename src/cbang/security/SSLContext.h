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

#ifndef CB_SSLCONTEXT_H
#define CB_SSLCONTEXT_H

#include <cbang/io/InputSource.h>

#include <string>

typedef struct ssl_ctx_st SSL_CTX;
typedef struct x509_store_st X509_STORE;
typedef struct bio_st BIO;

namespace cb {
  class SSL;
  class KeyPair;
  class Certificate;
  class CRL;

  class SSLContext {
    SSL_CTX *ctx;

  public:
    SSLContext();
    ~SSLContext();

    SSL_CTX *getCTX() const {return ctx;}
    X509_STORE *getStore() const;

    SSL *createSSL(BIO *bio);

    void setVerifyNone();
    void setVerifyPeer(bool verifyClientOnce = true,
                       bool failIfNoPeerCert = false, unsigned depth = 1);

    void addClientCA(const Certificate &cert);

    void useCertificate(const Certificate &cert);
    void addExtraChainCertificate(const Certificate &cert);
    void useCertificateChainFile(const std::string &filename);
    void usePrivateKey(const KeyPair &key);
    void usePrivateKey(const InputSource &source);

    void addTrustedCA(const Certificate &cert);
    void addTrustedCAStr(const std::string &data);
    void addTrustedCA(const InputSource &source);
    void addTrustedCA(BIO *bio);

    void addCRL(const CRL &crl);
    void addCRLStr(const std::string &data);
    void addCRL(const InputSource &source);
    void addCRL(BIO *bio);

    void setVerifyDepth(unsigned depth);
    void setCheckCRL(bool x = true);
  };
}

#endif // CB_SSLCONTEXT_H

