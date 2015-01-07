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

#include "SSL.h"

#include "SecurityUtilities.h"
#include "BStream.h"
#include "Certificate.h"

#include <cbang/os/Mutex.h>

#include <cbang/log/Logger.h>

#ifdef HAVE_VALGRIND_H
#include <valgrind/memcheck.h>
#endif

#include <iostream>

// This avoids a conflict with OCSP_RESPONSE in wincrypt.h
#ifdef OCSP_RESPONSE
#undef OCSP_RESPONSE
#endif

#define OPENSSL_THREAD_DEFINES
#include <openssl/opensslconf.h>

#if !defined(OPENSSL_THREADS)
#error "OpenSSL must be compiled with thread support"
#endif

#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/ssl.h>


using namespace std;
using namespace cb;


bool cb::SSL::initialized = false;
vector<Mutex *> cb::SSL::locks;


cb::SSL::SSL(SSL_CTX *ctx, BIO *bio) :
  ssl(0), renegotiateLimited(false), handshakes(0), state(PROCEED) {
  init();
  ssl = SSL_new(ctx);
  if (!ssl) THROW("Failed to create new SSL");
  if (bio) setBIO(bio);
  SSL_set_app_data(ssl, (char *)this);
}


cb::SSL::~SSL() {
  if (ssl) {
    SSL_free(ssl);
    ssl = 0;
  }
}


void cb::SSL::setBIO(BIO *bio) {
  SSL_set_bio(ssl, bio, bio);
}


string cb::SSL::getFullSSLErrorStr(int ret) const {
  string s = getSSLErrorStr(SSL_get_error(ssl, ret));
  if (peekError()) s += ", " + getErrorStr();
  return s;
}


bool cb::SSL::hasPeerCertificate() const {
  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) return false;
  X509_free(cert);
  return true;
}


void cb::SSL::verifyPeerCertificate() const {
  if (!hasPeerCertificate()) THROW("Peer did not present a certificate");

  if (SSL_get_verify_result(ssl) != X509_V_OK)
    THROW("Certificate does not verify");
}


SmartPointer<Certificate> cb::SSL::getPeerCertificate() const {
  if (SSL_get_verify_result(ssl) != X509_V_OK)
    THROW("Certificate does not verify");

  X509 *cert = SSL_get_peer_certificate(ssl);
  if (!cert) THROW("Peer did not present a certificate");

  return new Certificate(cert);
}


void cb::SSL::connect() {
  LOG_DEBUG(5, "cb::SSL::connect()");
  int ret = SSL_connect(ssl);

  if (ret == -1) {
    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      state = WANTS_CONNECT;
      return;
    }
  }

  state = PROCEED;

  if (ret != 1) THROWS("SSL connect failed: " << getFullSSLErrorStr(ret));
}


void cb::SSL::accept() {
  LOG_DEBUG(5, "cb::SSL::accept()");
  limitRenegotiation();
  int ret = SSL_accept(ssl);

  if (ret == -1) {
    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
      state = WANTS_ACCEPT;
      return;
    }
  }

  state = PROCEED;

  // We can ignore WANT_READ etc. here because the Socket layer already
  // retries reads and writes up to the specified timeout.
  if (ret != 1) {
    LOG_DEBUG(5, "SSL accept failed: " << getFullSSLErrorStr(ret));
    THROWS("SSL accept failed: " << getFullSSLErrorStr(ret));
  }
}


void cb::SSL::shutdown() {
  SSL_shutdown(ssl);
  LOG_DEBUG(5, "cb::SSL::shutdown() " << getErrorStr());

  flushErrors(); // Ignore errors
}


int cb::SSL::read(char *data, unsigned size) {
  LOG_DEBUG(5, "cb::SSL::read(" << size << ')');

  checkHandshakes();
  if (!checkWants() || !size) return 0;

  int ret = SSL_read(ssl, data, size);
  if (ret <= 0) {
    // Detect End of Stream
    if (SSL_get_shutdown(ssl) == SSL_RECEIVED_SHUTDOWN) return -1;

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return 0;

    string errMsg = getFullSSLErrorStr(ret);
    LOG_DEBUG(5, "cb::SSL::read() " << errMsg);
    THROWS("SSL read failed: " << errMsg);
  }

  LOG_DEBUG(5, "cb::SSL::read()=" << ret);

#ifdef VALGRIND_MAKE_MEM_DEFINED
  VALGRIND_MAKE_MEM_DEFINED(data, ret);
#endif // VALGRIND_MAKE_MEM_DEFINED

  return (unsigned)ret;
}


unsigned cb::SSL::write(const char *data, unsigned size) {
  LOG_DEBUG(5, "cb::SSL::write(" << size << ')');

  checkHandshakes();
  if (!checkWants() || !size) return 0;

  int ret = SSL_write(ssl, data, size);
  if (ret <= 0) {
    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) return 0;
    THROWS("SSL write failed: " << getFullSSLErrorStr(ret));
  }

  LOG_DEBUG(5, "cb::SSL::write()=" << ret);
  return (unsigned)ret;
}


void cb::SSL::lockingCallback(int mode, int n, const char *file, int line) {
  if ((int)locks.size() <= n)
    THROWS("Requested lock index " << n
           << " larger than CRYPTO_num_locks() at " << file << ":" << line);

  Mutex *lock = locks[n];
  if (!lock) locks[n] = lock = new Mutex;

  if (mode & CRYPTO_LOCK) lock->lock();
  else lock->unlock();
}


int cb::SSL::passwordCallback(char *buf, int num, int rwflags, void *data) {
  string msg = "Enter ";
  if (rwflags) msg += "encryption";
  else msg += "decryption";
  msg += " password: ";

  string pass = SecurityUtilities::getpass(msg);

  strncpy(buf, pass.c_str(), pass.length());
  return pass.length();
}


void cb::SSL::flushErrors() {
  while (getError()) continue;
}


unsigned cb::SSL::getError() {
  return ERR_get_error();
}


unsigned cb::SSL::peekError() {
  return ERR_peek_error();
}


unsigned cb::SSL::peekLastError() {
  return ERR_peek_last_error();
}


string cb::SSL::getErrorStr(unsigned err) {
  string result;
  char buffer[120];

  while (true) {
    if (!err) err = ERR_get_error();
    if (!err) break;

    ERR_error_string(err, buffer);
    err = 0;

    if (!result.empty()) result += ", ";
    result += buffer;
  }

  return result;
}


const char *cb::SSL::getSSLErrorStr(int err) {
  switch (err) {
  case SSL_ERROR_NONE: return "ERROR_NONE";
  case SSL_ERROR_SSL: return "ERROR_SSL";
  case SSL_ERROR_WANT_READ: return "ERROR_WANT_READ";
  case SSL_ERROR_WANT_WRITE: return "ERROR_WANT_WRITE";
  case SSL_ERROR_WANT_X509_LOOKUP: return "ERROR_WANT_X509_LOOKUP";
  case SSL_ERROR_SYSCALL: return "ERROR_SYSCALL";
  case SSL_ERROR_ZERO_RETURN: return "ERROR_ZERO_RETURN";
  case SSL_ERROR_WANT_CONNECT: return "ERROR_WANT_CONNECT";
  case SSL_ERROR_WANT_ACCEPT: return "ERROR_WANT_ACCEPT";
  default: return "UNKNOWN SSL ERROR";
  }
}


int cb::SSL::createObject(const string &oid, const string &shortName,
                          const string &longName) {
  int nid = OBJ_create(oid.c_str(), shortName.c_str(), longName.c_str());

  if (nid == NID_undef)
    THROWS("Failed to create SSL object oid='" << oid << ", SN='" << shortName
           << "', LN='" << longName << "': " << getErrorStr());

  return nid;
}


int cb::SSL::findObject(const string &name) {
  int nid = OBJ_txt2nid(name.c_str());
  if (nid == NID_undef) THROWS("Unrecognized SSL object '" << name << "'");
  return nid;
}


void cb::SSL::init() {
  if (initialized) return;

  SSL_library_init();
  SSL_load_error_strings();
  ERR_load_crypto_strings();
  OpenSSL_add_all_algorithms();

  locks.resize(CRYPTO_num_locks());
  CRYPTO_set_locking_callback(lockingCallback);

  initialized = true;
}


void cb::SSL::infoCallback(int where, int ret) {
  if (where & SSL_CB_HANDSHAKE_START) handshakes++;
}


namespace {
  extern "C" void ssl_info_callback(const ::SSL *ssl, int where, int ret) {
    ((cb::SSL *)SSL_get_app_data(ssl))->infoCallback(where, ret);
  }
}

void cb::SSL::limitRenegotiation() {
  handshakes = 0;

  if (renegotiateLimited) return;
  renegotiateLimited = true;

  SSL_set_info_callback(ssl, ssl_info_callback);
}


void cb::SSL::checkHandshakes() {
  if (3 < handshakes)
    THROW("Potential Client-Initiated Renegotiation DOS attack detected");
}


bool cb::SSL::checkWants() {
  switch (state) {
  case WANTS_ACCEPT: accept(); break;
  case WANTS_CONNECT: connect(); break;
  default: break;
  }

  return state == PROCEED;
}
