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

#include "BStream.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <openssl/bio.h>

#include <string.h>

using namespace cb;
using namespace std;


#if OPENSSL_VERSION_NUMBER < 0x10100000L
static BIO_METHOD *BIO_meth_new(int type, const char *name) {
  BIO_METHOD *method = (BIO_METHOD *)calloc(1, sizeof(BIO_METHOD));

  if (method) {
    method->type = type;
    method->name = name;
  }

  return method;
}


#define BIO_meth_set_write(b, f) b->bwrite = f
#define BIO_meth_set_read(b, f) b->bread = f
#define BIO_meth_set_puts(b, f) b->bputs = f
#define BIO_meth_set_gets(b, f) b->bgets = f
#define BIO_meth_set_ctrl(b, f) b->ctrl = f
#define BIO_meth_set_create(b, f) b->create = f
#define BIO_meth_set_destroy(b, f) b->destroy = f

#define BIO_set_init(b, val) b->init = val
#define BIO_set_data(b, val) b->ptr = val
#define BIO_get_data(b) b->ptr
#endif // OPENSSL_VERSION_NUMBER < 0x10100000L

#define BSTREAM_DEBUG() LOG_DEBUG(5, __FUNCTION__ << "()");


static int bstream_write(BIO *bio, const char *buf, int length) {
  BSTREAM_DEBUG();
  return ((BStream *)BIO_get_data(bio))->write(buf, length);
}


static int bstream_read(BIO *bio, char *buf, int length) {
  BSTREAM_DEBUG();
  return ((BStream *)BIO_get_data(bio))->read(buf, length);
}


static int bstream_puts(BIO *bio, const char *buf) {
  BSTREAM_DEBUG();
  return ((BStream *)BIO_get_data(bio))->puts(buf);
}


static int bstream_gets(BIO *bio, char *buf, int length) {
  BSTREAM_DEBUG();
  return ((BStream *)BIO_get_data(bio))->gets(buf, length);
}


static long bstream_ctrl(BIO *bio, int cmd, long sub, void *arg) {
  BSTREAM_DEBUG();
  return ((BStream *)BIO_get_data(bio))->ctrl(cmd, sub, arg);
}


static int bstream_create(BIO *bio) {
  BSTREAM_DEBUG();
  BIO_set_init(bio, 1);
  return 1;
}


static int bstream_destroy(BIO *bio) {
  BSTREAM_DEBUG();
  return ((BStream *)BIO_get_data(bio))->destroy();
}


BStream::BStream() {
  static BIO_METHOD *method = 0;

  if (!method) {
    method = BIO_meth_new(BIO_TYPE_FD, "iostream");
    if (!method) THROW("Failed to create BIO_METHOD object");

    BIO_meth_set_write(method, bstream_write);
    BIO_meth_set_read(method, bstream_read);
    BIO_meth_set_puts(method, bstream_puts);
    BIO_meth_set_gets(method, bstream_gets);
    BIO_meth_set_ctrl(method, bstream_ctrl);
    BIO_meth_set_create(method, bstream_create);
    BIO_meth_set_destroy(method, bstream_destroy);
  }

  bio = BIO_new(method);
  if (!bio) THROW("Failed to create BIO object");

  BIO_set_data(bio, this);
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
