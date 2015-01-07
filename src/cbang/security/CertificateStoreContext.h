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

#ifndef CB_CERTIFICATE_STORE_CONTEXT_H
#define CB_CERTIFICATE_STORE_CONTEXT_H

#include <time.h>

typedef struct x509_store_ctx_st X509_STORE_CTX;


namespace cb {
  class Certificate;
  class CertificateStore;
  class CertificateChain;

  class CertificateStoreContext {
    X509_STORE_CTX *ctx;

    CertificateStoreContext(const CertificateStoreContext &o) {}
    CertificateStoreContext &operator=(const CertificateStoreContext &o)
    {return *this;}

  public:
    CertificateStoreContext(X509_STORE_CTX *ctx);
    CertificateStoreContext(const CertificateStore &store,
                            const Certificate &cert);
    CertificateStoreContext(const CertificateStore &store,
                            const Certificate &cert,
                            const CertificateChain &chain);
    ~CertificateStoreContext();

    X509_STORE_CTX *getX509_STORE_CTX() const {return ctx;}

    void set(const Certificate &cert);
    void set(const CertificateChain &chain);

    int getError() const;
    int getErrorDepth() const;
    static const char *getErrorString(int error);

    void setPurpose(int purpose);
    void setTrust(int trust);
    void setPurposeInherit(int defPurpose, int purpose, int trust);
    void setFlags(unsigned long flags);
    void setTime(unsigned long flags, time_t t);

    void verify() const;
  };
}

#endif // CB_CERTIFICATE_STORE_CONTEXT_H

