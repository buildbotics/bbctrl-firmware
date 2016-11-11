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
#ifdef HAVE_OPENSSL

#include "BIOSocketImpl.h"

#include <cbang/util/DefaultCatch.h>

#include "Socket.h"

#include <openssl/bio.h>

using namespace cb;


BIOSocketImpl::BIOSocketImpl(Socket &socket) : socket(socket), ret(0) {
  bio->flags |= BIO_FLAGS_WRITE | BIO_FLAGS_READ;
}


int BIOSocketImpl::read(char *buf, int length) {
  try {
    try {
      ret = 0;
      exception = 0;
      ret = socket.read(buf, length);

      if (!ret && !socket.getBlocking()) bio->flags |= BIO_FLAGS_SHOULD_RETRY;
      else bio->flags &= ~BIO_FLAGS_SHOULD_RETRY;

      return ret ? ret : -1;

    } catch (const Socket::EndOfStream &) {
    } catch (const Exception &e) {
      exception = new Exception(e);
    }
  } CBANG_CATCH_ERROR;

  return -1;
}


int BIOSocketImpl::write(const char *buf, int length) {
  try {
    try {
      ret = 0;
      exception = 0;
      ret = socket.write(buf, length);

      if (!ret && !socket.getBlocking()) bio->flags |= BIO_FLAGS_SHOULD_RETRY;
      else bio->flags &= ~BIO_FLAGS_SHOULD_RETRY;

      return ret ? ret : -1;

    } catch (const Exception &e) {
      exception = new Exception(e);
      return -1;
    }
  } CBANG_CATCH_ERROR;

  return -1;
}

#endif // HAVE_OPENSSL
