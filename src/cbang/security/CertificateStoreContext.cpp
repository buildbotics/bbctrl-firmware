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

#include "CertificateStoreContext.h"

#include "SSL.h"
#include "Certificate.h"
#include "CertificateStore.h"
#include "CertificateChain.h"

#include <cbang/Exception.h>

#include <openssl/x509_vfy.h>

using namespace cb;


CertificateStoreContext::CertificateStoreContext(X509_STORE_CTX *ctx) :
  ctx(ctx) {
  if (!ctx) THROW("Certificate store context cannot be null");
}


CertificateStoreContext::CertificateStoreContext(const CertificateStore &store,
                                                 const Certificate &cert) :
  ctx(X509_STORE_CTX_new()) {
  if (!X509_STORE_CTX_init(ctx, store.getX509_STORE(), cert.getX509(), 0))
    THROWS("Failed to create certificate store context: "
           << SSL::getErrorStr());
}


CertificateStoreContext::
CertificateStoreContext(const CertificateStore &store, const Certificate &cert,
                        const CertificateChain &chain) :
  ctx(X509_STORE_CTX_new()) {
  if (!X509_STORE_CTX_init(ctx, store.getX509_STORE(), cert.getX509(),
                           chain.getX509_CHAIN()))
    THROWS("Failed to create certificate store context: "
           << SSL::getErrorStr());
}


CertificateStoreContext::~CertificateStoreContext() {
  if (ctx) X509_STORE_CTX_free(ctx);
}


void CertificateStoreContext::set(const Certificate &cert) {
  X509_STORE_CTX_set_cert(ctx, cert.getX509());
}


void CertificateStoreContext::set(const CertificateChain &chain) {
  X509_STORE_CTX_set_chain(ctx, chain.getX509_CHAIN());
}


int CertificateStoreContext::getError() const {
  return X509_STORE_CTX_get_error(ctx);
}


int CertificateStoreContext::getErrorDepth() const {
  return X509_STORE_CTX_get_error_depth(ctx);
}


const char *CertificateStoreContext::getErrorString(int error) {
  switch (error) {
  case X509_V_OK: return "OK";
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
    return "Unable to get issuer certificate";
  case X509_V_ERR_UNABLE_TO_GET_CRL: return "Unable to get CRL";
  case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
    return "Unable to decrypt certificate signature";
  case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
    return "Unable to decrypt CRL signature";
  case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
    return "Unable to decode issuer public key";
  case X509_V_ERR_CERT_SIGNATURE_FAILURE:
    return "Certificate signature failure";
  case X509_V_ERR_CRL_SIGNATURE_FAILURE: return "CRL signature failure";
  case X509_V_ERR_CERT_NOT_YET_VALID: return "Certificate not yet valid";
  case X509_V_ERR_CERT_HAS_EXPIRED: return "Certificate has expired";
  case X509_V_ERR_CRL_NOT_YET_VALID: return "CRL not yet valid";
  case X509_V_ERR_CRL_HAS_EXPIRED: return "CRL has expired";
  case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
    return "Error in certificate not before field";
  case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
    return "Error in certificate not after field";
  case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
    return "Error in CRL last update field";
  case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
    return "Error in CRL next update field";
  case X509_V_ERR_OUT_OF_MEM: return "Out of memory";
  case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
    return "Depth zero self signed certificate";
  case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    return "Self signed certificate in chain";
  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    return "Unable to get issuer certificate locally";
  case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
    return "Unable to verify leaf signature";
  case X509_V_ERR_CERT_CHAIN_TOO_LONG: return "Certificate chain too long";
  case X509_V_ERR_CERT_REVOKED: return "Certificate revoked";
  case X509_V_ERR_APPLICATION_VERIFICATION: return "Application verification";
  default: return "Unknown error";
  }
}


void CertificateStoreContext::setPurpose(int purpose) {
  if (!X509_STORE_CTX_set_purpose(ctx, purpose))
    THROWS("Failed to set certificate store context purpose: "
           << SSL::getErrorStr());
}


void CertificateStoreContext::setTrust(int trust) {
  if (!X509_STORE_CTX_set_trust(ctx, trust))
    THROWS("Failed to set certificate store context trust: "
           << SSL::getErrorStr());
}


void CertificateStoreContext::setPurposeInherit(int defPurpose, int purpose,
                                                int trust) {
  if (!X509_STORE_CTX_purpose_inherit(ctx, defPurpose, purpose, trust))
    THROWS("Certificate store context purpose inherit failed: "
           << SSL::getErrorStr());
}


void CertificateStoreContext::setFlags(unsigned long flags) {
  X509_STORE_CTX_set_flags(ctx, flags);
}


void CertificateStoreContext::setTime(unsigned long flags, time_t t) {
  X509_STORE_CTX_set_time(ctx, flags, t);
}


void CertificateStoreContext::verify() const {
  if (!X509_verify_cert(ctx))
    THROWS("Failed to verify certificate: " << getErrorString(getError())
           << ": " << SSL::getErrorStr());
}
