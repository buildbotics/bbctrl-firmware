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

#include "SSLContext.h"

#include "SSL.h"
#include "BIStream.h"
#include "KeyPair.h"
#include "Certificate.h"
#include "CRL.h"

#include <cbang/Exception.h>

// This avoids a conflict with OCSP_RESPONSE in wincrypt.h
#ifdef OCSP_RESPONSE
#undef OCSP_RESPONSE
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

using namespace std;
using namespace cb;


#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define TLS_method TLSv1_method
#endif // OPENSSL_VERSION_NUMBER < 0x10100000L


SSLContext::SSLContext() : ctx(0) {
  cb::SSL::init();

  ctx = SSL_CTX_new(TLS_method());
  if (!ctx) THROWS("Failed to create SSL context: " << cb::SSL::getErrorStr());

  SSL_CTX_set_default_passwd_cb(ctx, cb::SSL::passwordCallback);

  // A session ID is required for session caching to work
  SSL_CTX_set_session_id_context(ctx, (unsigned char *)"cbang", 5);
}


SSLContext::~SSLContext() {
  if (ctx) {
    SSL_CTX_free(ctx);
    ctx = 0;
  }
}


X509_STORE *SSLContext::getStore() const {
  X509_STORE *store = SSL_CTX_get_cert_store(ctx);
  if (!store) THROW("Could not get SSL context certificate store");

  return store;
}


SmartPointer<cb::SSL> SSLContext::createSSL(BIO *bio) {
  return new cb::SSL(ctx, bio);
}


void SSLContext::setVerifyNone() {
  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);
}


void SSLContext::setVerifyPeer(bool verifyClientOnce, bool failIfNoPeerCert,
                               unsigned depth) {
  int mode = SSL_VERIFY_PEER;
  if (verifyClientOnce) mode |= SSL_VERIFY_CLIENT_ONCE;
  if (failIfNoPeerCert) mode |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
  SSL_CTX_set_verify(ctx, mode, 0);
  if (depth) SSL_CTX_set_verify_depth(ctx, depth);
}


void SSLContext::addClientCA(const Certificate &cert) {
  if (!SSL_CTX_add_client_CA(ctx, X509_dup(cert.getX509())))
    THROWS("Failed to add client CA: " << cb::SSL::getErrorStr());
}


void SSLContext::useCertificate(const Certificate &cert) {
  if (!SSL_CTX_use_certificate(ctx, X509_dup(cert.getX509())))
    THROWS("Failed to use certificate: " << cb::SSL::getErrorStr());
}


void SSLContext::addExtraChainCertificate(const Certificate &cert) {
  if (!SSL_CTX_add_extra_chain_cert(ctx, X509_dup(cert.getX509())))
    THROWS("Failed to add extra chain certificate: " << cb::SSL::getErrorStr());
}


void SSLContext::useCertificateChainFile(const string &filename) {
  if (!(SSL_CTX_use_certificate_chain_file(ctx, filename.c_str())))
    THROWS("Failed to load certificate chain file '" << filename
           << "': " << cb::SSL::getErrorStr());
}


void SSLContext::usePrivateKey(const KeyPair &key) {
  if (!(SSL_CTX_use_PrivateKey(ctx, key.getEVP_PKEY())))
    THROWS("Failed to use private key: " << cb::SSL::getErrorStr());
}


void SSLContext::usePrivateKey(const InputSource &source) {
  KeyPair pri;
  source.getStream() >> pri;
  usePrivateKey(pri);
}


void SSLContext::addTrustedCA(const Certificate &cert) {
  X509_STORE *store = getStore();
  if (!X509_STORE_add_cert(store, X509_dup(cert.getX509())))
    THROWS("Failed to add certificate to store " << cb::SSL::getErrorStr());
}


void SSLContext::addTrustedCAStr(const string &data) {
  addTrustedCA(InputSource(data.c_str(), data.length()));
}


void SSLContext::addTrustedCA(const InputSource &source) {
  BIStream bio(source.getStream());
  addTrustedCA(bio.getBIO());
}


void SSLContext::addTrustedCA(BIO *bio) {
  // TODO free X509
  X509 *cert = PEM_read_bio_X509(bio, 0, cb::SSL::passwordCallback, 0);
  if (!cert) THROWS("Failed to read certificate " << cb::SSL::getErrorStr());

  X509_STORE *store = getStore();
  if (!X509_STORE_add_cert(store, cert))
    THROWS("Failed to add certificate to store " << cb::SSL::getErrorStr());
}


void SSLContext::addCRL(const CRL &crl) {
  X509_STORE *store = getStore();

  // Add CRL
  if (!X509_STORE_add_crl(store, crl.getX509_CRL()))
    THROWS("Error adding CRL" << cb::SSL::getErrorStr());

  setCheckCRL(true);
}


void SSLContext::addCRLStr(const string &data) {
  addCRL(InputSource(data.c_str(), data.length()));
}


void SSLContext::addCRL(const InputSource &source) {
  BIStream bio(source.getStream());
  addCRL(bio.getBIO());
}


void SSLContext::addCRL(BIO *bio) {
  X509_STORE *store = getStore();

  // Read CRL
  X509_CRL *crl = PEM_read_bio_X509_CRL(bio, 0, 0, 0);
  if (!crl || cb::SSL::peekError())
    THROWS("Error reading CRL " << cb::SSL::getErrorStr());

  // Add CRL
  if (!X509_STORE_add_crl(store, crl))
    THROWS("Error adding CRL" << cb::SSL::getErrorStr());

  setCheckCRL(true);
}


void SSLContext::setVerifyDepth(unsigned depth) {
  SSL_CTX_set_verify_depth(ctx, depth);
}


void SSLContext::setCheckCRL(bool x) {
  X509_STORE *store = getStore();

  // Enable CRL checking
  X509_VERIFY_PARAM *param = X509_VERIFY_PARAM_new();
  X509_VERIFY_PARAM_set_flags(param, X509_V_FLAG_CRL_CHECK);
  X509_STORE_set1_param(store, param);
  X509_VERIFY_PARAM_free(param);
}
