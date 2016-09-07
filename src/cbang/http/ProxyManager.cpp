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

#include "ProxyManager.h"

#include "Request.h"
#include "Response.h"
#include "Header.h"
#include "Transaction.h"

#include <cbang/config/Options.h>

#include <cbang/net/Base64.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/Digest.h>
#endif

#include <cbang/util/Random.h>
#include <cbang/log/Logger.h>
#include <cbang/util/StringMap.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


ProxyManager::ProxyManager(Inaccessible) : enable(false), initialized(false) {
  uint64_t x = Random::instance().rand<uint64_t>();
  cnonce = Base64().encode((char *)&x, sizeof(x));
}


void ProxyManager::addOptions(Options &options) {
  options.pushCategory("Network");
  options.addTarget("proxy-enable", enable, "Enable proxy configuration");
  options.addTarget("proxy", proxy, "Set proxy for outgoing HTTP connections");
  options.addTarget("proxy-user", user, "Set user name for proxy connections");
  SmartPointer<Option> opt =
    options.addTarget("proxy-pass", pass, "Set password for proxy connections");
  opt->setObscured();
  options.popCategory();
}


const IPAddress &ProxyManager::getAddress() const {
  if (enabled() && !address.getIP()) address = IPAddress(proxy);
  return address;
}


void ProxyManager::connect(Transaction &transaction) {
  if (!enabled() || !hasCredentials() || initialized) return;

  // Authenticate with proxy
  URI uri(string("http://") + transaction.getAddress().getHost() + "/");
  Request request(RequestMethod::HTTP_OPTIONS, uri);
  Response &response = transaction.exchange(request);

  // Flush data
  unsigned length = response.getContentLength();
  char buffer[4096];
  while (length) {
    unsigned size = length < 4096 ? length : 4096;
    transaction.read(buffer, size);
    length -= size;
  }
}


void ProxyManager::authenticate(Response &response) {
  if (!enabled()) return;

  if (response.getStatus() == StatusCode::HTTP_PROXY_AUTHENTICATION_REQUIRED &&
      response.has("Proxy-Authenticate")) {
    string auth = response["Proxy-Authenticate"];

    size_t start = auth.find_first_not_of(" \t\n\r");
    size_t end = auth.find_first_of(" \t\n\r", start);
    if (start == string::npos || end == string::npos)
      THROWS("Invalid Proxy-Authenticate");

    scheme = String::toLower(auth.substr(start, end - start));
    if (scheme == "basic") initialized = true;

#ifdef HAVE_OPENSSL
    else if (scheme == "digest") {
      StringMap challenge;
      Header::parseKeyValueList(auth.substr(end), challenge);

      if (!challenge.has("realm"))
        THROW("Invalid Proxy-Authenticate, missing realm");
      realm = challenge["realm"];

      if (!challenge.has("nonce"))
        THROW("Invalid Proxy-Authenticate, missing nonce");
      if (nonce != challenge["nonce"]) {
        nonceCount = 0;
        nonce = challenge["nonce"];
      }

      if (challenge.has("opaque")) opaque = challenge["opaque"];
      else opaque.clear();
      if (challenge.has("algorithm")) {
        algorithm = String::toUpper(challenge["algorithm"]);
        if (Digest::hasAlgorithm(String::toLower(algorithm)))
          THROWS("Unsupported proxy digest authentication algorithm: "
                 << algorithm);
      } else algorithm.clear();

      if (challenge.has("qop")) {
        bool ok = false;
        vector<string> tokens;
        String::tokenize(challenge["qop"], tokens, " \t\n\r,");
        for (unsigned i = 0; i < tokens.size(); i++)
          if (tokens[i] == "auth") ok = true;

        if (!ok) THROW("Only support 'auth' proxy quality of protection");
        qop = "auth";

      } else qop.clear();

      initialized = true;
    }
#endif // HAVE_OPENSSL

    else THROWS("Unsupported proxy authorization scheme: " << scheme);

  } else if (response.has("Proxy-Authentication-Info")) {
    StringMap info;
    Header::parseKeyValueList(response["Proxy-Authentication-Info"], info);
    nonce = info["nextnonce"];
  }
}


void ProxyManager::addCredentials(Request &request) {
  if (!enabled() || !hasCredentials() || !initialized) return;

  if (scheme == "basic") {
    string auth = "Basic " + Base64().encode(user + ":" + pass);
    request.set("Proxy-Authorization", auth);

#ifdef HAVE_OPENSSL
  } else if (scheme == "digest") {
    string algorithm = String::toLower(this->algorithm);
    string ha1 = Digest::hashHex(user + ':' + realm + ':' + pass, algorithm);
    string ha2 = Digest::hashHex(string(request.getMethod().toString()) + ':' +
                                 request.getURI().toString(), algorithm);

    string auth =
      "Digest username=" + Header::quoted(user) +
      ", realm=" + Header::quoted(realm) +
      ", nonce=" + Header::quoted(nonce) +
      ", uri=" + request.getURI().toString();

    if (!algorithm.empty()) auth += ", algorithm=" + algorithm;
    if (!opaque.empty()) auth += ", opaque=" + Header::quoted(opaque);

    string digest = ha1 + ':' + nonce + ':';

    if (!qop.empty()) {
      string nonceCountStr = String::printf("%08x", ++nonceCount);

      digest += nonceCountStr + ':' + cnonce + ':' + qop + ':';
      auth +=
        ", qop=" + qop +
        ", nc=" + nonceCountStr +
        ", cnonce=" + Header::quoted(cnonce);
    }

    digest = Digest::hashHex(digest + ha2, algorithm);
    auth += ", response=\"" + digest + '"';

    request.set("Proxy-Authorization", auth);
#endif // HAVE_OPENSSL
  }
}
