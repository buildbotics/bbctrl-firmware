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

#include <cbang/net/IPAddress.h>

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/log/Logger.h>

#include <cbang/socket/Socket.h>

#include <cbang/os/SysError.h>

#if _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <string.h>

using namespace std;
using namespace cb;


IPAddress::IPAddress(const string &host) :
  host(host), ip(0), port(portFromString(host)) {

  Socket::initialize(); // Windows needs this

  size_t ptr = host.find(":");
  if (ptr != string::npos) this->host = host.substr(0, ptr);

  if (this->host == "0") this->host = "0.0.0.0"; // OSX needs this
}


IPAddress::IPAddress(const string &host, uint16_t port) :
  host(host), ip(0), port(port) {

  Socket::initialize(); // Windows needs this

  if (host.find(":") != string::npos)
    THROWS("Address '" << host << "' already has port.");

  if (this->host == "0") this->host = "0.0.0.0"; // OSX needs this
}


const string &IPAddress::getHost() const {
  if (host.empty())
    const_cast<IPAddress *>(this)->host =
      String(ip >> 24) + "." + String((ip >> 16) & 0xff) + "." +
      String((ip >> 8) & 0xff) + "." + String(ip & 0xff);

  return host;
}


uint32_t IPAddress::getIP() const {
  if (!ip && !host.empty())
    const_cast<IPAddress *>(this)->ip = ipFromString(host);
  return ip;
}


string IPAddress::toString() const {
  return getHost() + (getPort() ? String::printf(":%d", getPort()) : string());
}


uint32_t IPAddress::ipFromString(const string &host) {
  vector<IPAddress> addrs;

  if (!ipsFromString(host, addrs, 0, 1))
    THROWS("Could not get IP address for " << host);

  return addrs[0].getIP();
}


unsigned IPAddress::ipsFromString(const string &host, vector<IPAddress> &addrs,
                                  uint16_t port, unsigned max) {
  // TODO support IPv6 addresses

  Socket::initialize(); // Windows needs this

  string hostname;
  size_t ptr = host.find_last_of(":");
  if (ptr != string::npos) {
    hostname = host.substr(0, ptr);
    port = String::parseU16(host.substr(ptr + 1));

  } else hostname = host;

  struct addrinfo hints;
  struct addrinfo *res = 0;
  int err;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_INET; // Force IPv4 for now
  hints.ai_socktype = SOCK_STREAM;

  if ((err = getaddrinfo(hostname.c_str(), 0, &hints, &res)))
    THROWS("Could not get IP address for " << hostname << ": "
           << gai_strerror(err));

  unsigned count = 0;
  struct addrinfo *info;
  for (info = res; info && (!max || count < max); info = info->ai_next) {
    if (!info->ai_addr) continue;
    uint32_t ip = ntohl(((struct sockaddr_in *)info->ai_addr)->sin_addr.s_addr);
    addrs.push_back(IPAddress(ip, port));
    count++;
  }

  freeaddrinfo(res);

  return count;
}


const string IPAddress::hostFromIP(const IPAddress &ip) {
  // Force 127.0.0.1 to localhost
  if (ip.getIP() == 0x7f000001) return "localhost";

  Socket::initialize(); // Windows needs this

  struct sockaddr_in addr;
  char buffer[1024];

  // TODO support IPv6
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ip.getPort());
  addr.sin_addr.s_addr = htonl(ip.getIP());

  if (getnameinfo((struct sockaddr *)&addr, sizeof(addr), buffer,
                  sizeof(buffer), 0, 0, 0))
    THROWS("Reverse lookup for " << ip << " failed: " << SysError());

  return buffer;
}


uint16_t IPAddress::portFromString(const string &host) {
  size_t ptr = host.find_last_of(":");
  if (ptr == string::npos) return 0;
  return String::parseU16(host.substr(ptr + 1));
}
