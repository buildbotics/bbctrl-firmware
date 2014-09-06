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

#include "Revoked.h"
#include "SSL.h"
#include "Extension.h"

#include <openssl/x509.h>

#include <cbang/Exception.h>

using namespace std;
using namespace cb;


Revoked::Revoked() : rev(X509_REVOKED_new()), deallocate(true) {}


Revoked::~Revoked() {
  if (deallocate) X509_REVOKED_free(rev);
}


void Revoked::setDate(uint64_t ts) {
  ASN1_TIME *tm = ASN1_TIME_new();
  X509_gmtime_adj(tm, ts);

  if (!X509_REVOKED_set_revocationDate(rev, tm)) {
    ASN1_TIME_free(tm);
    THROWS("Failed to set revocation date: " << SSL::getErrorStr());
  }

  ASN1_TIME_free(tm);
}


void Revoked::setReason(const string &reason) {
  if (reason == "unspecified") return;

  const char *reasons[] = {
	"unspecified",
	"keyCompromise",
	"CACompromise",
	"affiliationChanged",
	"superseded",
	"cessationOfOperation",
	"certificateHold",
    "removeFromCRL",
    "privilegeWithdrawn",
    "aACompromise",
    0
  };

  ASN1_ENUMERATED *a = ASN1_ENUMERATED_new();

  for (int i = 0; reasons[i]; i++)
    if (reason == reasons[i]) {
      ASN1_ENUMERATED_set(a, i);
      break;
    }

  if (!X509_REVOKED_add1_ext_i2d(rev, NID_crl_reason, a, 0, 0)) {
    THROWS("Failed to add extension 'CRLReason'='" << reason
           << "': " << SSL::getErrorStr());
   	ASN1_ENUMERATED_free(a);
  }

  ASN1_ENUMERATED_free(a);
}


void Revoked::setSerial(long number) {
  ASN1_INTEGER *serial = ASN1_INTEGER_new();
  ASN1_INTEGER_set(serial, number);

  if (!X509_REVOKED_set_serialNumber(rev, serial))
    THROWS("Failed to set revoked serial number: " << SSL::getErrorStr());

  ASN1_INTEGER_free(serial);
}
