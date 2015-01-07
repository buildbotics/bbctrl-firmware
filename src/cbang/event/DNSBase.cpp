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

#include "DNSBase.h"
#include "Base.h"

#include <cbang/Exception.h>
#include <cbang/util/DefaultCatch.h>

#include <event2/dns.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


namespace {
  void dns_cb(int error, char type, int count, int ttl, void *addresses,
              void *arg) {
    vector<IPAddress> addrs;
    DNSCallback &cb = *(DNSCallback *)arg;

    try {
      if (error == DNS_ERR_NONE) {
        if (type == DNS_IPv4_A) {
          uint32_t *ips = (uint32_t *)addresses;

          for (int i = 0; i < count; i++) {
            addrs.push_back(IPAddress(ips[i]));
            addrs.back().setHost(cb.getSource().getHost());
          }

        } else if (type == DNS_PTR) {
          addrs.push_back(IPAddress((const char *)addresses));
          addrs.back().setIP(cb.getSource().getIP());

        } else {
          LOG_ERROR("Unsupported DNS response type " << type);
          error = -1;
        }
      }

      cb(error, addrs, ttl);

    } CATCH_ERROR;

    cb.self.release();
  }
}


DNSBase::DNSBase(Base &base, bool initialize, bool failRequestsOnExit) :
  dns(evdns_base_new(base.getBase(),
                     (initialize ? EVDNS_BASE_INITIALIZE_NAMESERVERS : 0) |
                     EVDNS_BASE_DISABLE_WHEN_INACTIVE)),
  failRequestsOnExit(failRequestsOnExit) {
  if (!dns) THROW("Failed to create DNSBase");
}


DNSBase::~DNSBase() {
  if (dns) evdns_base_free(dns, failRequestsOnExit);
}


void DNSBase::addNameserver(const IPAddress &ns) {
  evdns_base_nameserver_add(dns, ns.getIP());
}


DNSRequest DNSBase::resolve(const string &name,
                            const SmartPointer<DNSCallback> &cb, bool search) {
  cb->setSource(IPAddress(name));

  evdns_request *req =
    evdns_base_resolve_ipv4
    (dns, name.c_str(), (search ? 0 : DNS_QUERY_NO_SEARCH), dns_cb, cb.get());

  if (!req) THROW("DNS lookup failed");

  cb->self = cb;

  return DNSRequest(dns, req);
}


DNSRequest DNSBase::reverse(uint32_t ip, const SmartPointer<DNSCallback> &cb,
                            bool search) {
  cb->setSource(IPAddress(ip));

  struct in_addr addr = {ip};

  evdns_request *req =
    evdns_base_resolve_reverse
    (dns, &addr, (search ? 0 : DNS_QUERY_NO_SEARCH), dns_cb, cb.get());

  if (!req) THROW("DNS reverse lookup failed");

  cb->self = cb;

  return DNSRequest(dns, req);
}


const char *DNSBase::getErrorStr(int error) {
  switch (error) {
  case -1:                   return "Unsupported response type";
  case DNS_ERR_NONE:         return "None";
  case DNS_ERR_CANCEL:       return "Canceled";
  case DNS_ERR_FORMAT:       return "format error";
  case DNS_ERR_NODATA:       return "No data";
  case DNS_ERR_NOTEXIST:     return "Does not exist";
  case DNS_ERR_NOTIMPL:      return "Not implemented";
  case DNS_ERR_REFUSED:      return "Refused";
  case DNS_ERR_SERVERFAILED: return "Server failed";
  case DNS_ERR_SHUTDOWN:     return "Shutdown";
  case DNS_ERR_TIMEOUT:      return "Timed out";
  case DNS_ERR_TRUNCATED:    return "Truncated";
  default:                   return "Unknown";
  }
}
