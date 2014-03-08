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

#include "BStream.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <openssl/bio.h>

#include <string.h>

using namespace cb;
using namespace std;


#define BSTREAM_DEBUG() LOG_DEBUG(5, __FUNCTION__ << "()");


static int bstream_write(BIO *bio, const char *buf, int length) {
  BSTREAM_DEBUG();
  return ((BStream *)bio->ptr)->write(buf, length);
}


static int bstream_read(BIO *bio, char *buf, int length) {
  BSTREAM_DEBUG();
  return ((BStream *)bio->ptr)->read(buf, length);
}


static int bstream_puts(BIO *bio, const char *buf) {
  BSTREAM_DEBUG();
  return ((BStream *)bio->ptr)->puts(buf);
}


static int bstream_gets(BIO *bio, char *buf, int length) {
  BSTREAM_DEBUG();
  return ((BStream *)bio->ptr)->gets(buf, length);
}


static long bstream_ctrl(BIO *bio, int cmd, long sub, void *arg) {
  BSTREAM_DEBUG();
  return ((BStream *)bio->ptr)->ctrl(cmd, sub, arg);
}


static int bstream_create(BIO *bio) {
  BSTREAM_DEBUG();

  bio->init = 1;
  bio->num = 0;
  bio->ptr = 0;
  bio->flags = 0;

  return 1;
}


static int bstream_destroy(BIO *bio) {
  BSTREAM_DEBUG();
  return ((BStream *)bio->ptr)->destroy();
}


static BIO_METHOD BStreamMethods = {
  BIO_TYPE_FD,
  "iostream",
  bstream_write,
  bstream_read,
  bstream_puts,
  bstream_gets,
  bstream_ctrl,
  bstream_create,
  bstream_destroy,
  0,
};


BStream::BStream() {
  bio = BIO_new(&BStreamMethods);
  if (!bio) THROW("Failed to create BIO object");
  bio->ptr = this;
}


BStream::~BStream() {
  if (bio) {
    BIO_free(bio);
    bio = 0;
  }
}


int BStream::write(const char *buf, int length) {
  return -2; // Not implemented
}


int BStream::read(char *buf, int length) {
  return -2; // Not implemented
}


int BStream::puts(const char *buf) {
  return write(buf, strlen(buf));
}


int BStream::gets(char *buf, int length) {
  LOG_DEBUG(3, "BStream::gets(" << length << ")");
  return -2; // Not implemented
}


long BStream::ctrl(int cmd, long sub, void *arg) {
  const char *cmdStr;

  switch (cmd) {
  case BIO_CTRL_RESET: cmdStr = "RESET"; break;
  case BIO_CTRL_EOF: cmdStr = "EOF"; break;
  case BIO_CTRL_INFO: cmdStr = "INFO"; break;
  case BIO_CTRL_SET: cmdStr = "SET"; break;
  case BIO_CTRL_GET: cmdStr = "GET"; break;
  case BIO_CTRL_PUSH: cmdStr = "PUSH"; break;
  case BIO_CTRL_POP: cmdStr = "POP"; break;
  case BIO_CTRL_GET_CLOSE: cmdStr = "GET_CLOSE"; break;
  case BIO_CTRL_SET_CLOSE: cmdStr = "SET_CLOSE"; break;
  case BIO_CTRL_PENDING: cmdStr = "PENDING"; break;
  case BIO_CTRL_FLUSH: cmdStr = "FLUSH"; break;
  case BIO_CTRL_DUP: cmdStr = "DUP"; break;
  case BIO_CTRL_WPENDING: cmdStr = "WPENDING"; break;
  case BIO_CTRL_SET_CALLBACK: cmdStr = "SET_CALLBACK"; break;
  case BIO_CTRL_GET_CALLBACK: cmdStr = "GET_CALLBACK"; break;
  case BIO_CTRL_SET_FILENAME: cmdStr = "SET_FILENAME"; break;
  case BIO_CTRL_DGRAM_CONNECT: cmdStr = "DGRAM_CONNECT"; break;
  case BIO_CTRL_DGRAM_SET_CONNECTED: cmdStr = "DGRAM_SET_CONNECTED"; break;
  case BIO_CTRL_DGRAM_SET_RECV_TIMEOUT:
    cmdStr = "DGRAM_SET_RECV_TIMEOUT"; break;
  case BIO_CTRL_DGRAM_GET_RECV_TIMEOUT:
    cmdStr = "DGRAM_GET_RECV_TIMEOUT"; break;
  case BIO_CTRL_DGRAM_SET_SEND_TIMEOUT:
    cmdStr = "DGRAM_SET_SEND_TIMEOUT"; break;
  case BIO_CTRL_DGRAM_GET_SEND_TIMEOUT:
    cmdStr = "DGRAM_GET_SEND_TIMEOUT"; break;
  case BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP:
    cmdStr = "DGRAM_GET_RECV_TIMER_EXP"; break;
  case BIO_CTRL_DGRAM_GET_SEND_TIMER_EXP:
    cmdStr = "DGRAM_GET_SEND_TIMER_EXP"; break;
  case BIO_CTRL_DGRAM_MTU_DISCOVER: cmdStr = "DGRAM_MTU_DISCOVER"; break;
  case BIO_CTRL_DGRAM_QUERY_MTU: cmdStr = "DGRAM_QUERY_MTU"; break;
  case BIO_CTRL_DGRAM_GET_MTU: cmdStr = "DGRAM_GET_MTU"; break;
  case BIO_CTRL_DGRAM_SET_MTU: cmdStr = "DGRAM_SET_MTU"; break;
  case BIO_CTRL_DGRAM_MTU_EXCEEDED: cmdStr = "DGRAM_MTU_EXCEEDED"; break;
#ifdef BIO_CTRL_DGRAM_GET_PEER
  case BIO_CTRL_DGRAM_GET_PEER: cmdStr = "DGRAM_GET_PEER"; break;
#endif
#ifdef BIO_CTRL_DGRAM_SET_PEER
  case BIO_CTRL_DGRAM_SET_PEER: cmdStr = "DGRAM_SET_PEER"; break;
#endif
#ifdef BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT
  case BIO_CTRL_DGRAM_SET_NEXT_TIMEOUT:
    cmdStr = "DGRAM_SET_NEXT_TIMEOUT"; break;
#endif
  default: cmdStr = "UNKNOWN"; break;
  }

  LOG_DEBUG(5, "BStream::ctrl(" << cmdStr << '=' << cmd << ", " << sub << ")");
  return 1; // Success
}


int BStream::destroy() {
  bio = 0;
  return 1;
}
