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

#include "WebServer.h"
#include "HTTP.h"
#include "Request.h"

#include <cbang/config/Options.h>
#include <cbang/log/Logger.h>
#include <cbang/config/Options.h>
#include <cbang/os/SystemUtilities.h>

#ifdef HAVE_OPENSSL
#include <cbang/openssl/SSLContext.h>
#else
namespace cb {class SSLContext {};}
#endif

using namespace std;
using namespace cb::Event;

namespace {
  class MarkRequestSecure : public HTTPHandler {
    WebServer &server;

  public:
    MarkRequestSecure(WebServer &server) : server(server) {}


    // From HTTPHandler
    Request *createRequest(evhttp_request *req) {
      return server.createRequest(req);
    }


    bool operator()(Request &req) {
      req.setSecure(true);
      return server(req);
    }
  };
}


WebServer::WebServer(cb::Options &options, const Base &base,
                     const cb::SmartPointer<cb::SSLContext> &sslCtx,
                     const cb::SmartPointer<HTTPHandlerFactory> &factory) :
  HTTPHandlerGroup(factory), options(options), sslCtx(sslCtx),
  http(new HTTP(base)), https(sslCtx.isNull() ? 0 : new HTTP(base, sslCtx)),
  initialized(false) {

  SmartPointer<Option> opt;
  options.pushCategory("HTTP Server");

  opt = options.add("http-addresses", "A space separated list of server "
                    "address and port pairs to listen on in the form "
                    "<ip | hostname>[:<port>]");
  opt->setType(Option::STRINGS_TYPE);
  opt->setDefault("0.0.0.0:80");

  options.add("allow", "Client addresses which are allowed to connect to this "
              "server.  This option overrides IPs which are denied in the "
              "deny option.  The pattern 0/0 matches all addresses."
              )->setDefault("0/0");
  options.add("deny", "Client address which are not allowed to connect to this "
              "server.")->setType(Option::STRINGS_TYPE);

  options.add("http-max-body-size", "Maximum size of an HTTP request body.");
  options.add("http-max-headers-size", "Maximum size of the HTTP request "
              "headers.");
  options.add("http-server-timeout", "Maximum time in seconds before an http "
              "request times out.");

  options.popCategory();

  if (!https.isNull()) {
    options.pushCategory("HTTP Server SSL");
    opt = options.add("https-addresses", "A space separated list of secure "
                      "server address and port pairs to listen on in the form "
                      "<ip | hostname>[:<port>]");
    opt->setType(Option::STRINGS_TYPE);
    opt->setDefault("0.0.0.0:443");
    options.add("crl-file", "Supply a Certificate Revocation List.  Overrides "
                "any internal CRL");
    options.add("certificate-file", "The servers certificate file in PEM "
                "format.")->setDefault("certificate.pem");
    options.add("private-key-file", "The servers private key file in PEM "
                "format.")->setDefault("private.pem");
    options.popCategory();
  }
}


WebServer::~WebServer() {}


void WebServer::init() {
  if (initialized) return;
  initialized = true;

  // IP filters
  ipFilter.allow(options["allow"]);
  ipFilter.deny(options["deny"]);

  // Configure ports
  Option::strings_t addresses = options["http-addresses"].toStrings();
  for (unsigned i = 0; i < addresses.size(); i++)
    addListenPort(addresses[i]);

  // Configure HTTP
  if (options["http-max-body-size"].hasValue())
    setMaxBodySize(options["http-max-body-size"].toInteger());
  if (options["http-max-headers-size"].hasValue())
    setMaxHeadersSize(options["http-max-headers-size"].toInteger());
  if (options["http-server-timeout"].hasValue())
    setTimeout(options["http-server-timeout"].toInteger());

#ifdef HAVE_OPENSSL
  // SSL
  if (!https.isNull()) {
    // Configure secure ports
    addresses = options["https-addresses"].toStrings();
    for (unsigned i = 0; i < addresses.size(); i++)
      addSecureListenPort(addresses[i]);

    // Load server certificate
    // TODO should load file relative to configuration file
    if (options["certificate-file"].hasValue()) {
      string certFile = options["certificate-file"].toString();
      if (SystemUtilities::exists(certFile))
        sslCtx->useCertificateChainFile(certFile);
      else LOG_WARNING("Certificate file not found " << certFile);
    }

    // Load server private key
    if (options["private-key-file"].hasValue()) {
      string priKeyFile = options["private-key-file"].toString();
      if (SystemUtilities::exists(priKeyFile))
        sslCtx->usePrivateKey(priKeyFile);
      else LOG_WARNING("Private key file not found " << priKeyFile);
    }

    // CRL
    if (options["crl-file"].hasValue()) {
      string crlFile = options["crl-file"];
      if (SystemUtilities::exists(crlFile)) sslCtx->addCRL(crlFile);
      else LOG_WARNING("Certificate Relocation List not found " << crlFile);
    }

    https->setGeneralCallback(new MarkRequestSecure(*this));
  }
#endif // HAVE_OPENSSL

  http->setGeneralCallback(SmartPointer<HTTPHandler>::Null(this));
}


bool WebServer::allow(Request &req) const {
  return ipFilter.isAllowed(req.getClientIP().getIP());
}


bool WebServer::operator()(Request &req) {
  if (!allow(req)) THROWX("Unauthorized", HTTP_UNAUTHORIZED);
  return HTTPHandlerGroup::operator()(req);
}


void WebServer::addListenPort(const cb::IPAddress &addr) {
  LOG_INFO(1, "Listening for HTTP on " << addr);
  http->bind(addr);
  ports.push_back(addr);
}


void WebServer::addSecureListenPort(const cb::IPAddress &addr) {
  LOG_INFO(1, "Listening for HTTPS on " << addr);
  https->bind(addr);
  securePorts.push_back(addr);
}


void WebServer::setMaxBodySize(unsigned size) {
  http->setMaxBodySize(size);
  if (!https.isNull()) https->setMaxBodySize(size);
}


void WebServer::setMaxHeadersSize(unsigned size) {
  http->setMaxHeadersSize(size);
  if (!https.isNull()) https->setMaxHeadersSize(size);
}


void WebServer::setTimeout(int timeout) {
  http->setTimeout(timeout);
  if (!https.isNull()) https->setTimeout(timeout);
}
