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

#ifndef CBANG_SOCKET_H
#define CBANG_SOCKET_H

#include "SocketImpl.h"

#include <ios>

namespace cb {
  class SSLContext;

  /// Create a TCP socket connection.
  class Socket {
    static bool initialized;

    SocketImpl *impl;

    // Don't allow copy constructor or assignment
    Socket(const Socket &o) : impl(0) {}
    Socket &operator=(const Socket &o) {return *this;}

  public:
    // NOTE Inheriting from std::exception instead of cb::Exception due to
    // problems with matching the correct catch in SocketDevice::read() and
    // perhaps elsewhere.  Presumably this is caused by a compiler bug.
    class EndOfStream : public std::exception {
    public:
      EndOfStream() {}

      // From std::exception
      virtual const char *what() const throw() {return "End of stream";}
    };


    enum {
      NONBLOCKING = 1 << 0,
      PEEK        = 1 << 1,
    };


    Socket();
    Socket(const SmartPointer<SSLContext> &sslCtx);
    virtual ~Socket();

    static void initialize();

    virtual SocketImpl *getImpl() {return impl;}
    virtual bool isSecure() {return impl->isSecure();}

    /// @return True if the socket is open.
    virtual bool isOpen() const {return impl->isOpen();}
    virtual bool isConnected() const {return canWrite(0);}

    virtual bool canRead(double timeout = 0) const;
    virtual bool canWrite(double timeout = 0) const;

    virtual void setReuseAddr(bool reuse) {impl->setReuseAddr(reuse);}
    virtual void setBlocking(bool blocking) {impl->setBlocking(blocking);}
    virtual bool getBlocking() const {return impl->getBlocking();}
    virtual void setKeepAlive(bool keepAlive) {impl->setKeepAlive(keepAlive);}
    virtual void setSendBuffer(int size) {impl->setSendBuffer(size);}
    virtual void setReceiveBuffer(int size) {impl->setReceiveBuffer(size);}
    virtual void setReceiveTimeout(double timeout)
    {impl->setReceiveTimeout(timeout);}
    virtual void setSendTimeout(double timeout) {impl->setSendTimeout(timeout);}
    virtual void setTimeout(double timeout);

    virtual void open() {impl->open();}
    virtual void bind(const IPAddress &ip) {impl->bind(ip);}
    virtual void listen(int backlog = -1) {impl->listen(backlog);}
    virtual SmartPointer<Socket> accept(IPAddress *ip = 0)
    {return impl->accept(ip);}

    /// Connect to the specified address and port.
    virtual void connect(const IPAddress &ip) {impl->connect(ip);}

    /// Write data to an open connection.
    virtual std::streamsize write(const char *data, std::streamsize length,
                                  unsigned flags = 0);

    /// Read data from an open connection.
    virtual std::streamsize read(char *data, std::streamsize length,
                                 unsigned flags = 0);

    /// Close an open connection.
    virtual void close() {impl->close();}

    virtual socket_t get() const {return impl->get();}
  };
}

#endif // CBANG_SOCKET_H
