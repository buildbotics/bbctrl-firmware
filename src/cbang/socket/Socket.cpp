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

#include "Socket.h"

#include "SocketDefaultImpl.h"
#include "SocketSSLImpl.h"
#include "SocketDebugImpl.h"
#include "SocketDebugger.h"
#include "SocketSet.h"
#include "SocketType.h" // For winsock.h

#include <cbang/Exception.h>
#include <cbang/Zap.h>
#include <cbang/String.h>
#include <cbang/openssl/SSL.h>
#include <cbang/openssl/SSLContext.h>

#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>

using namespace std;
using namespace cb;

bool Socket::initialized = false;


Socket::Socket() {
  if (SocketDebugger::instance().isEnabled())
    impl = new SocketDebugImpl(this);
  else impl = new SocketDefaultImpl(this);
}


Socket::Socket(const SmartPointer<SSLContext> &sslCtx) {
  if (SocketDebugger::instance().isEnabled())
    impl = new SocketDebugImpl(this);

#ifdef HAVE_OPENSSL
  else impl = new SocketSSLImpl(this, sslCtx);
#else
  else THROW("C! not built with OpenSSL support");
#endif
}


Socket::~Socket() {
  close();
  zap(impl);
}


void Socket::initialize() {
  if (initialized) return;

#ifdef _WIN32
  WSADATA wsa;

  if (WSAStartup(MAKEWORD(2, 2), &wsa) != NO_ERROR)
    THROW("Failed to start winsock");

  if (LOBYTE(wsa.wVersion) != 2 || HIBYTE(wsa.wVersion) != 2)
    THROW("Error need winsock version 2.2");
#endif

  initialized = true;
}


void Socket::setTimeout(double timeout) {
  setReceiveTimeout(timeout);
  setSendTimeout(timeout);
}


bool Socket::canRead(double timeout) const {
  if (!isOpen()) return false;
  SocketSet set;
  set.add(*this, SocketSet::READ);
  return set.select(timeout);
}


bool Socket::canWrite(double timeout) const {
  if (!isOpen()) return false;
  SocketSet set;
  set.add(*this, SocketSet::WRITE);
  return set.select(timeout);
}


streamsize Socket::write(const char *data, streamsize length, unsigned flags) {
  if (!isOpen()) THROW("Socket not open");
  bool blocking = !(flags & NONBLOCKING) && getBlocking();
  LOG_DEBUG(5, "Socket start " << (blocking ? "" : "non-")
            << "blocking write " << length);

  streamsize bytes = impl->write(data, length, flags);

  LOG_DEBUG(5, "Socket write " << bytes);
  if (bytes) LOG_DEBUG(6, String::hexdump(data, bytes));

  return bytes;
}


streamsize Socket::read(char *data, streamsize length, unsigned flags) {
  if (!isOpen()) THROW("Socket not open");
  bool blocking = !(flags & NONBLOCKING) && getBlocking();
  LOG_DEBUG(5, "Socket start " << (blocking ? "" : "non-")
            << "blocking read " << length);

  streamsize bytes = impl->read(data, length, flags);

  LOG_DEBUG(5, "Socket read " << bytes);

  // NOTE SocketDevice expects this exact message
  if (bytes == -1) THROW("End of stream");

  if (bytes) LOG_DEBUG(6, String::hexdump(data, bytes));

  return bytes;
}
