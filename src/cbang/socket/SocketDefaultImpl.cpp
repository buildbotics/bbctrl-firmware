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

#include "SocketDefaultImpl.h"

#include "Socket.h"
#include "SocketType.h"
#include "SocketSet.h"
#include "SocketDebugger.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SysError.h>
#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>

#include <cbang/Exception.h>
#include <cbang/String.h>

#ifdef _WIN32
typedef int socklen_t;  // Unix socket length
#define MSG_DONTWAIT 0
#define MSG_NOSIGNAL 0
#define SHUT_RDWR 2
#define EINPROGRESS WSAEWOULDBLOCK

#else // _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define INVALID_SOCKET -1        // WinSock invalid socket
#define SOCKET_ERROR   -1        // Basic WinSock error

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
#endif

#include <errno.h>
#include <time.h>
#include <string.h>

using namespace std;
using namespace cb;


SocketDefaultImpl::SocketDefaultImpl(Socket *parent, SSLContext *sslCtx) :
  SocketImpl(parent, sslCtx), socket(INVALID_SOCKET), blocking(true),
  connected(false) {
  Socket::initialize();
}


bool SocketDefaultImpl::isOpen() const {
  return socket != INVALID_SOCKET;
}


void SocketDefaultImpl::open() {
  if (isOpen()) THROW("Socket already open");

  if ((socket = (int)::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) ==
      INVALID_SOCKET)
    THROW("Failed to create socket");
}


void SocketDefaultImpl::setReuseAddr(bool reuse) {
  if (!isOpen()) open();

#ifdef _WIN32
  BOOL opt = reuse;
#else
  int opt = reuse;
#endif

  SysError::clear();
  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                 sizeof(opt)))
    THROWS("Failed to set reuse addr: " << SysError());
}


void SocketDefaultImpl::setBlocking(bool blocking) {
  if (!isOpen()) open();

#ifdef _WIN32
  u_long on = blocking ? 0 : 1;
  ioctlsocket((socket_t)socket, FIONBIO, &on);

#else
  int opts = fcntl(socket, F_GETFL);
  if (opts >= 0) {
    if (blocking) opts &= ~O_NONBLOCK;
    else opts |= O_NONBLOCK;

    fcntl(socket, F_SETFL, opts);
  }
#endif

  this->blocking = blocking;
}


void SocketDefaultImpl::setKeepAlive(bool keepAlive) {
  if (!isOpen()) open();

#ifdef _WIN32
  BOOL opt = keepAlive;
#else
  int opt = keepAlive;
#endif

  SysError::clear();
  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt,
                 sizeof(opt)))
    THROWS("Failed to set socket keep alive: " << SysError());
}


void SocketDefaultImpl::setSendBuffer(int size) {
  if (!isOpen()) open();

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_SNDBUF, (char *)&size,
                 sizeof(size)))
    THROWS("Could not set send buffer to " << size << ": " << SysError());
}


void SocketDefaultImpl::setReceiveBuffer(int size) {
  if (!isOpen()) open();

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_RCVBUF, (char *)&size,
                 sizeof(size)))
    THROWS("Could not set receive buffer to " << size << ": " << SysError());
}


void SocketDefaultImpl::setSendTimeout(double timeout) {
  if (!isOpen()) open();

#ifdef _WIN32
  DWORD t = 1000 * timeout; // ms
#else
  struct timeval t = Timer::toTimeVal(timeout);
#endif

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&t,
                 sizeof(t)))
    THROWS("Could not set send timeout to " << timeout << ": " << SysError());
}


void SocketDefaultImpl::setReceiveTimeout(double timeout) {
  if (!isOpen()) open();

#ifdef _WIN32
  DWORD t = 1000 * timeout; // ms
#else
  struct timeval t = Timer::toTimeVal(timeout);
#endif

  if (setsockopt((socket_t)socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&t,
                 sizeof(t)))
    THROWS("Could not set receive timeout to " << timeout << ": "
           << SysError());
}


void SocketDefaultImpl::bind(const IPAddress &ip) {
  if (!isOpen()) open();

  struct sockaddr_in addr;

  memset((void *)&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_port        = htons(ip.getPort());
  addr.sin_addr.s_addr = htonl((unsigned)ip ? (unsigned)ip : INADDR_ANY);

  SysError::clear();
  if (::bind((socket_t)socket, (struct sockaddr *)&addr, sizeof(addr)) ==
      SOCKET_ERROR)
    THROWS("Could not bind socket to " << ip << ": " << SysError());
}


void SocketDefaultImpl::listen(int backlog) {
  if (!isOpen()) open();

  SysError::clear();
  if (::listen((socket_t)socket, backlog == -1 ? SOMAXCONN : backlog) ==
      SOCKET_ERROR)
    THROW("listen failed");

#ifndef _WIN32
  fcntl(socket, F_SETFD, FD_CLOEXEC);
#endif
}


SmartPointer<Socket> SocketDefaultImpl::accept(IPAddress *ip) {
  if (!isOpen()) open();

  SmartPointer<Socket> a = createSocket();
  SocketDefaultImpl *aSock = dynamic_cast<SocketDefaultImpl *>(a->getImpl());

  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);

  if ((aSock->socket =
       (int)::accept((socket_t)socket, (struct sockaddr *)&addr, &len)) !=
      INVALID_SOCKET) {
    IPAddress inAddr = ntohl(addr.sin_addr.s_addr);
    inAddr.setPort(ntohs(addr.sin_port));

    if (ip) *ip = inAddr;

    aSock->connected = true;
    aSock->capture(inAddr, true);
    aSock->setBlocking(blocking);

    return a;
  }

  return 0;
}


void SocketDefaultImpl::connect(const IPAddress &ip) {
  if (!isOpen()) open();

  LOG_INFO(3, "Connecting to " << ip);

  try {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)ip.getPort());
    sin.sin_addr.s_addr = htonl(ip);

    SysError::clear();
    if (::connect((socket_t)socket, (struct sockaddr *)&sin, sizeof(sin)) == -1)
      if (SysError::get() != EINPROGRESS)
        THROWS("Failed to connect to " << ip << ": " << SysError());

    connected = true;
    capture(ip, false);

  } catch (const Exception &e) {
    close();

    throw;
  }
}


streamsize SocketDefaultImpl::write(const char *data, streamsize length,
                                    unsigned flags) {
  if (!isOpen()) THROW("Socket not open");
  if (!length) return 0;

  int f = MSG_NOSIGNAL;
  if (flags & Socket::NONBLOCKING) f |= MSG_DONTWAIT;

  SysError::clear();
  streamsize ret = send((socket_t)socket, data, length, f);
  int err = SysError::get();

  LOG_DEBUG(5, "send() = " << ret << " of " << length);

  if (ret < 0) {
#ifdef _WIN32
    // NOTE: send() can return -1 even when there is no error
    if (!err || err == WSAEWOULDBLOCK || err == WSAENOBUFS) return 0;
#else
    if (err == EAGAIN) return 0;
#endif

    THROWS("Send error: " << err << ": " << SysError(err));
  }

  if (out.get()) out->write(data, ret); // Capture

  return ret;
}


streamsize SocketDefaultImpl::read(char *data, streamsize length,
                                   unsigned flags) {
  if (!isOpen()) THROW("Socket not open");
  if (!length) return 0;

  int f = MSG_NOSIGNAL;
  if (flags & Socket::NONBLOCKING) f |= MSG_DONTWAIT;
  if (flags & Socket::PEEK) f |= MSG_PEEK;

  SysError::clear();
  streamsize ret = recv((socket_t)socket, data, length, f);
  int err = SysError::get();

  LOG_DEBUG(5, "recv() = " << ret << " of " << length);

  if (!ret) return -1; // Orderly shutdown
  if (ret < 0) {
#ifdef _WIN32
    // NOTE: Windows can return -1 even when there is no error
    if (!err || err == WSAEWOULDBLOCK) return 0;
#else
    if (err == ECONNRESET) return -1;
    if (err == EAGAIN || err == EWOULDBLOCK) return 0;
#endif

    THROWS("Receive error: " << err << ": " << SysError(err));
  }

  if (in.get()) in->write(data, ret); // Capture

  return ret;
}


void SocketDefaultImpl::close() {
  if (!isOpen()) return;

  // If socket was connected call shutdown()
  if (connected) {
    shutdown(socket, SHUT_RDWR);
    connected = false;
  }

#ifdef _WIN32
  closesocket((SOCKET)socket);
#else
  ::close(socket);
#endif

  in = out = 0; // Flush capture

  socket = INVALID_SOCKET;
}


void SocketDefaultImpl::capture(const IPAddress &addr, bool incoming) {
  SocketDebugger &debugger = SocketDebugger::instance();
  if (!debugger.getCapture()) return;

  const string &dir = debugger.getCaptureDirectory();
  SystemUtilities::ensureDirectory(dir);

  uint64_t id = debugger.getNextConnectionID();

  string prefix = dir + "/" + String(id) + "-" + (incoming ? "in" : "out") +
    "-" + addr.toString() + "-";

#ifdef _WIN32
  prefix = String::replace(prefix, ':', '-');
#endif

  string request = prefix + "request.dat";
  string response = prefix + "response.dat";

  in =
    SystemUtilities::open(incoming ? request : response, ios::out | ios::trunc);
  out =
    SystemUtilities::open(incoming ? response : request, ios::out | ios::trunc);
}
