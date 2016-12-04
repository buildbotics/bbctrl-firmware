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

#include "CSR.h"

#include "SSL.h"
#include "BIStream.h"
#include "BOStream.h"
#include "KeyPair.h"
#include "Extension.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include <sstream>

using namespace cb;
using namespace std;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
#define X509_EXTENSION_get_object(e) (e)->object
#define X509_EXTENSION_get_data(e) (e)->value
#define ASN1_STRING_get0_data(e) M_ASN1_STRING_data(e)
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */


struct CSR::private_t {
  STACK_OF(X509_EXTENSION) *exts;
  private_t() : exts(0) {}
};


CSR::CSR() : csr(0), pri(new private_t) {
  SSL::init();
  csr = X509_REQ_new();
}


CSR::CSR(const string &pem) : csr(0), pri(new private_t) {
  SSL::init();
  csr = X509_REQ_new();
  istringstream stream(pem);
  read(stream);
}


CSR::~CSR() {
  if (csr) X509_REQ_free(csr);
  if (pri->exts) sk_X509_EXTENSION_pop_free(pri->exts, X509_EXTENSION_free);
  delete pri;
}


void CSR::getPublicKey(KeyPair &key) {
  EVP_PKEY *pkey = X509_REQ_get_pubkey(csr);
  if (!pkey)
    THROWS("Error getting public key from CSR: " << SSL::getErrorStr());

  if (key.getEVP_PKEY()) EVP_PKEY_free(key.getEVP_PKEY());
  key.setEVP_PKEY(pkey);
}


SmartPointer<KeyPair> CSR::getPublicKey() {
  SmartPointer<KeyPair> key = new KeyPair(0);
  getPublicKey(*key);
  return key;
}


void CSR::addNameEntry(const string &name, const string &value) {
  if (!X509_NAME_add_entry_by_txt(X509_REQ_get_subject_name(csr), name.c_str(),
                                  MBSTRING_ASC, (uint8_t *)value.c_str(), -1,
                                  -1, 0))
    THROWS("Failed to add CSR name entry '" << name << "'='" << value
           << "': " << SSL::getErrorStr());
}


string CSR::getNameEntry(const string &name) const {
  X509_NAME *subject = X509_REQ_get_subject_name(csr);

  for (int i = 0; i < X509_NAME_entry_count(subject); i++) {
    X509_NAME_ENTRY *entry = X509_NAME_get_entry(subject, i);
    ASN1_OBJECT *obj = X509_NAME_ENTRY_get_object(entry);
    int nid = OBJ_obj2nid(obj);

    if (OBJ_nid2sn(nid) == name)
      return (char *)ASN1_STRING_get0_data(X509_NAME_ENTRY_get_data(entry));
  }

  THROWS("Name entry '" << name << "' not found");
}


bool CSR::hasExtension(const string &name) {
  ASN1_OBJECT *obj = OBJ_txt2obj(name.c_str(), 0);
  if (!obj) THROWS("Unrecognized extension name '" << name << "'");

  STACK_OF(X509_EXTENSION) *exts = X509_REQ_get_extensions(csr);
  if (!exts) return false;

  bool result = false;
  for (int i = 0; i < sk_X509_EXTENSION_num(exts); i++) {
    X509_EXTENSION *ext = sk_X509_EXTENSION_value(exts, i);
    if (OBJ_cmp(X509_EXTENSION_get_object(ext), obj) == 0) {
      result = true;
      break;
    }
  }

  sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);

  return result;
}


string CSR::getExtension(const string &name) {
  ASN1_OBJECT *obj = OBJ_txt2obj(name.c_str(), 0);
  if (!obj) THROWS("Unrecognized extension name '" << name << "'");

  string value;
  bool found = false;

  STACK_OF(X509_EXTENSION) *exts = X509_REQ_get_extensions(csr);

  if (exts) {
    for (int i = 0; i < sk_X509_EXTENSION_num(exts); i++) {
      X509_EXTENSION *ext = sk_X509_EXTENSION_value(exts, i);
      if (OBJ_cmp(X509_EXTENSION_get_object(ext), obj) == 0) {
        found = true;

        ostringstream stream;
        BOStream bio(stream);

        if (!X509V3_EXT_print(bio.getBIO(), ext, 0, 0))
          ASN1_STRING_print(bio.getBIO(), X509_EXTENSION_get_data(ext));

        value = stream.str();

        break;
      }
    }

    sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);
  }

  if (!found) THROWS("Extension '" << name << "' not in CSR");

  return value;
}


void CSR::addExtension(const string &name, const string &value) {
  Extension ext(name, value);

  if (!pri->exts) pri->exts = sk_X509_EXTENSION_new_null();
  sk_X509_EXTENSION_push(pri->exts, ext.get());

  ext.setDeallocate(false);
}


void CSR::sign(const KeyPair &key, const string &digest) {
  const EVP_MD *md = EVP_get_digestbyname(digest.c_str());
  if (!md) THROWS("Unrecognized message digest '" << digest << "'");

  if (!X509_REQ_set_pubkey(csr, key.getEVP_PKEY()))
    THROWS("Failed to set CSR pub key: " << SSL::getErrorStr());

  if (pri->exts && !X509_REQ_add_extensions(csr, pri->exts))
    THROWS("Failed to add extensions: " << SSL::getErrorStr());

  if (!X509_REQ_sign(csr, key.getEVP_PKEY(), md))
    THROWS("Failed to sign CSR: " << SSL::getErrorStr());
}


void CSR::verify() {
  EVP_PKEY *pkey = X509_REQ_get_pubkey(csr);
  if (!pkey)
    THROWS("Error getting public key from CSR: " << SSL::getErrorStr());

  if (!X509_REQ_verify(csr, pkey)) {
    EVP_PKEY_free(pkey);
    THROWS("CSR failed verification: " << SSL::getErrorStr());
  }

  EVP_PKEY_free(pkey);
}



string CSR::toString() const {
  return SSTR(*this);
}


istream &CSR::read(istream &stream) {
  BIStream bio(stream);

  X509_REQ_free(csr); // Allocate new

  csr = PEM_read_bio_X509_REQ(bio.getBIO(), 0, 0, 0);
  if (!csr) THROWS("Failed to read CSR: " << SSL::getErrorStr());

  return stream;
}


ostream &CSR::write(ostream &stream) const {
  BOStream bio(stream);

  if (!PEM_write_bio_X509_REQ_NEW(bio.getBIO(), csr))
    THROWS("Failed to write CSR: " << SSL::getErrorStr());

  return stream;
}


ostream &CSR::print(ostream &stream) const {
  BOStream bio(stream);

  if (!X509_REQ_print(bio.getBIO(), csr))
    THROWS("Failed to print CSR: " << SSL::getErrorStr());

  return stream;
}
