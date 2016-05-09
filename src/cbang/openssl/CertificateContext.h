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

#ifndef CB_CERTIFICATE_CONTEXT_H
#define CB_CERTIFICATE_CONTEXT_H

typedef struct v3_ext_ctx X509V3_CTX;

namespace cb {
  class Certificate;
  class CSR;
  class CRL;

  class CertificateContext {
    X509V3_CTX *ctx;

  public:
    CertificateContext(const Certificate *issuer = 0,
                       const Certificate *subject = 0,
                       const CSR *request = 0, const CRL *crl = 0,
                       int flags = 0);
    ~CertificateContext();

    X509V3_CTX *getX509V3_CTX() const {return ctx;}
  };
}

#endif // CB_CERTIFICATE_CONTEXT_H
