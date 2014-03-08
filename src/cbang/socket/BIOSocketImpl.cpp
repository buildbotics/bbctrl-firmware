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
      return ret = socket.read(buf, length);

    } catch (const Exception &e) {
      exception = new Exception(e);
      return -1;
    }
  } CBANG_CATCH_ERROR;

  return -1;
}


int BIOSocketImpl::write(const char *buf, int length) {
  try {
    try {
      ret = 0;
      exception = 0;
      return ret = socket.write(buf, length);

    } catch (const Exception &e) {
      exception = new Exception(e);
      return -1;
    }
  } CBANG_CATCH_ERROR;

  return -1;
}
