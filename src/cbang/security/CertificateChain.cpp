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

#include "CertificateChain.h"

#include "Certificate.h"

#include <openssl/x509_vfy.h>

using namespace cb;


CertificateChain::CertificateChain(const CertificateChain &o) {
  chain = sk_X509_dup(o.chain);
  for (int i = 0; i < sk_X509_num(chain); i++)
    CRYPTO_add(&(sk_X509_value(chain, i)->references), 1, CRYPTO_LOCK_EVP_PKEY);
}


CertificateChain::CertificateChain(X509_CHAIN *chain) : chain(chain) {
  if (!chain) this->chain = sk_X509_new_null();
}


CertificateChain::~CertificateChain() {
  sk_X509_pop_free(chain, X509_free);
}


CertificateChain &CertificateChain::operator=(const CertificateChain &o) {
  sk_X509_pop_free(chain, X509_free);

  chain = sk_X509_dup(o.chain);
  for (int i = 0; i < sk_X509_num(chain); i++)
    CRYPTO_add(&(sk_X509_value(chain, i)->references), 1, CRYPTO_LOCK_EVP_PKEY);

  return *this;
}


void CertificateChain::add(Certificate &cert) {
  CRYPTO_add(&(cert.getX509()->references), 1, CRYPTO_LOCK_EVP_PKEY);
  sk_X509_push(chain, cert.getX509());
}
