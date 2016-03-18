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

#include "SocketSet.h"

#include "Socket.h"
#include "SocketType.h"
#include "SocketDebugger.h"

#include <cbang/Exception.h>
#include <cbang/SmartPointer.h>
#include <cbang/Zap.h>

#include <cbang/time/Timer.h>

#include <cbang/os/SysError.h>

#ifdef _WIN32
#define FD_SETSIZE 4096
#include <winsock2.h>

#else
#include <sys/select.h>
#include <sys/types.h>
#endif

using namespace cb;

struct SocketSet::private_t {
  fd_set read;
  fd_set write;
  fd_set except;
};


SocketSet::SocketSet() : p(new private_t) {
  Socket::initialize();
  clear();
}


SocketSet::SocketSet(const SocketSet &s) : p(new private_t), maxFD(s.maxFD) {
  *p = *s.p;
}


SocketSet::~SocketSet() {
  zap(p);
}


void SocketSet::clear() {
  FD_ZERO(&p->read);
  FD_ZERO(&p->write);
  FD_ZERO(&p->except);
  maxFD = -1;
}


void SocketSet::add(const Socket &socket, int type) {
  socket_t s = (socket_t)socket.get();

  if (type & READ) FD_SET(s, &p->read);
  if (type & WRITE) FD_SET(s, &p->write);
  if (type & EXCEPT) FD_SET(s, &p->except);
  if (maxFD < (int)s) maxFD = s;
}


void SocketSet::remove(const Socket &socket, int type) {
  socket_t s = (socket_t)socket.get();

  if (type & READ) FD_CLR(s, &p->read);
  if (type & WRITE) FD_CLR(s, &p->write);
  if (type & EXCEPT) FD_CLR(s, &p->except);
}


bool SocketSet::isSet(const Socket &socket, int type) const {
  socket_t s = (socket_t)socket.get();

  return
    ((type & READ)   && FD_ISSET(s, &p->read))  ||
    ((type & WRITE)  && FD_ISSET(s, &p->write)) ||

    (!SocketDebugger::instance().isEnabled() &&
     ((type & EXCEPT) && FD_ISSET(s, &p->except)));
}


bool SocketSet::select(double timeout) {
  if (SocketDebugger::instance().isEnabled()) return true;

  struct timeval t;
  if (0 <= timeout) t = Timer::toTimeVal(timeout);

  SysError::clear();
  int ret =
    ::select(maxFD + 1, &p->read, &p->write, &p->except, 0 <= timeout ? &t : 0);

  if (ret < 0) THROWS("select() " << SysError());
  return ret;
}
