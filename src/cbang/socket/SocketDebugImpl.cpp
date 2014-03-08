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

#include "SocketDebugImpl.h"

#include "Socket.h"
#include "SocketDebugger.h"
#include "SocketDebugConnection.h"

#include <cbang/Exception.h>
#include <cbang/Zap.h>

using namespace std;
using namespace cb;


SocketDebugImpl::~SocketDebugImpl() {
  close();
}


void SocketDebugImpl::bind(const IPAddress &ip) {
  open();
  bound = true;
  this->ip = ip;
}


SmartPointer<Socket> SocketDebugImpl::accept(IPAddress *ip) {
  if (!isOpen()) THROW("Socket not open");
  if (!bound) THROW("Socket not bound");

  SocketDebugConnection *con = SocketDebugger::instance().accept(this->ip);

  if (con) {
    if (ip) *ip = con->addr;

    SmartPointer<Socket> socket = createSocket();
    SocketDebugImpl *impl = dynamic_cast<SocketDebugImpl *>(socket->getImpl());
    if (impl) {
      impl->con = con;
      impl->open();

    } else THROW("Expected SocketDebugImpl");

    return socket;
  }

  return 0;
}


void SocketDebugImpl::connect(const IPAddress &ip) {
  if (!isOpen()) open();

  this->ip = ip;
  con = SocketDebugger::instance().connect(ip);
  deleteCon = true; // Delete the SocketDebugConnection on connects
}


streamsize SocketDebugImpl::write(const char *data, streamsize length,
                                unsigned flags) {
  if (!isOpen()) THROW("Socket not open");
  if (!con || !con->out) return length;

  con->out->write(data, length);
  con->out->flush();
  if (con->out->fail()) THROW("Failed to write to debug socket output");

  return length;
}


streamsize SocketDebugImpl::read(char *data, streamsize length,
                                 unsigned flags) {
  if (!isOpen()) THROW("Socket not open");
  if (!con || !con->in) return -1;
  if (!length) return 0;

  con->in->read(data, length);
  streamsize size = con->in->gcount();

  if (!size && con->in->eof()) return -1;

  return size;
}


void SocketDebugImpl::close() {
  socketOpen = listening = bound = false;
  if (deleteCon) zap(con);
  if (con) con->markDone();
  con = 0;
}
