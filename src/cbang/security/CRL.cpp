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

#include "CRL.h"

#include "SSL.h"
#include "KeyPair.h"
#include "Certificate.h"
#include "Extension.h"
#include "Revoked.h"
#include "BIStream.h"
#include "BOStream.h"

#include <cbang/Exception.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <sstream>

using namespace cb;
using namespace std;


CRL::CRL(X509_CRL *crl) : crl(crl) {
  SSL::init();
  if (!crl)
    if (!(this->crl = X509_CRL_new()))
      THROWS("Failed to create new CRL: " << SSL::getErrorStr());

}


CRL::CRL(const string &pem) : crl(0) {
  SSL::init();
  if (!(this->crl = X509_CRL_new()))
    THROWS("Failed to create new CRL: " << SSL::getErrorStr());
  istringstream stream(pem);
  read(stream);
}


CRL::~CRL() {
  if (crl) X509_CRL_free(crl);
}


void CRL::setVersion(int version) {
  if (!X509_CRL_set_version(crl, version))
    THROWS("Failed to set CRL version: " << SSL::getErrorStr());
}


int CRL::getVersion() const {
  return X509_CRL_get_version(crl);
}


void CRL::setIssuer(const Certificate &cert) {
  X509_NAME *name = X509_get_subject_name(cert.getX509());
  if (!name) THROWS("Failed to get issuer name: " << SSL::getErrorStr());

  if (!X509_CRL_set_issuer_name(crl, name))
    THROWS("Failed to set issuer name: " << SSL::getErrorStr());
}


void CRL::setLastUpdate(uint64_t x) {
  ASN1_TIME *tm = ASN1_TIME_new();
  if (!tm) THROW("Unable to allocate ANS1 time");

  X509_gmtime_adj(tm, x);

  if (!X509_CRL_set_lastUpdate(crl, tm)) {
    ASN1_TIME_free(tm);
    THROWS("Failed to set CRL's last update: " << SSL::getErrorStr());
  }

  ASN1_TIME_free(tm);
}


void CRL::setNextUpdate(uint64_t x) {
  ASN1_TIME *tm = ASN1_TIME_new();
  if (!tm) THROW("Unable to allocate ANS1 time");

  X509_gmtime_adj(tm, x);

  if (!X509_CRL_set_nextUpdate(crl, tm)) {
    ASN1_TIME_free(tm);
    THROWS("Failed to set CRL's next update: " << SSL::getErrorStr());
  }

  ASN1_TIME_free(tm);
}


bool CRL::hasExtension(const string &name) {
  return 0 <= X509_CRL_get_ext_by_NID(crl, SSL::findObject(name), -1);
}


string CRL::getExtension(const string &name) {
  X509_EXTENSION *ext =
    X509_CRL_get_ext(crl,
                     X509_CRL_get_ext_by_NID(crl, SSL::findObject(name), -1));
  if (!ext) THROWS("Extension '" << name << "' not in CRL");

  ostringstream stream;
  BOStream bio(stream);

  if (!X509V3_EXT_print(bio.getBIO(), ext, 0, 0))
    M_ASN1_OCTET_STRING_print(bio.getBIO(), ext->value);

  return stream.str();
}


void CRL::addExtension(const string &name, const string &value) {
  Extension ext(name, value);

  if (!X509_CRL_add_ext(crl, ext.get(), -1))
    THROWS("Failed to add extension '" << name << "'='" << value
           << "': " << SSL::getErrorStr());
}


void CRL::revoke(const Certificate &cert, const std::string &reason,
                 uint64_t ts) {
  Revoked rev;
  rev.setSerial(cert.getSerial());
  rev.setDate(ts);
  rev.setReason(reason);

  if (!X509_CRL_add0_revoked(crl, rev.get()))
    THROWS("Failed to add revoke: " << SSL::getErrorStr());

  rev.setDeallocate(false);
}


bool CRL::wasRevoked(const Certificate &cert) const {
  X509_REVOKED *rev = 0;
  X509_CRL_get0_by_cert(crl, &rev, cert.getX509());
  return rev;
}


void CRL::sort() {
  X509_CRL_sort(crl);
}


void CRL::sign(KeyPair &key, const string &digest) {
  OpenSSL_add_all_digests();

  const EVP_MD *md = EVP_get_digestbyname(digest.c_str());
  if (!md) THROWS("Unrecognized message digest '" << digest << "'");

  if (!X509_CRL_sign(crl, key.getEVP_PKEY(), md))
    THROWS("Failed to sign CRL: " << SSL::getErrorStr());

}


void CRL::verify(KeyPair &key) {
  if (!X509_CRL_verify(crl, key.getEVP_PKEY()))
    THROWS("CRL failed verification: " << SSL::getErrorStr());
}


string CRL::toString() const {
  return SSTR(*this);
}


istream &CRL::read(istream &stream) {
  BIStream bio(stream);

  if (!PEM_read_bio_X509_CRL(bio.getBIO(), &crl, 0, 0))
    THROWS("Failed to read CRL: " << SSL::getErrorStr());

  return stream;
}


ostream &CRL::write(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_X509_CRL(bio.getBIO(), crl))
    THROWS("Failed to write CRL: " << SSL::getErrorStr());

  return stream;
}
