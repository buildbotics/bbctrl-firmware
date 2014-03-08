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

#include "BIMemory.h"

#include <cbang/log/Logger.h>

#include <string.h>

#include <openssl/bio.h>

using namespace std;
using namespace cb;


BIMemory::BIMemory(const char *data, uint64_t length) :
  data(data), length(length), readPos(0) {
  bio->flags |= BIO_FLAGS_READ;
}


int BIMemory::read(char *buf, int length) {
  int fill = readPos < this->length ? this->length - readPos : 0;
  if (length && !fill) return -1;
  if (fill < length) length = fill;
  if (!length) return 0;

  memcpy(buf, data + readPos, length);
  readPos += length;

  return length;
}


int BIMemory::gets(char *buf, int length) {
  if (!length) return 0;
  length--; // Space for '\0'

  const char *begin = data + readPos;
  const char *end = data + this->length;
  const char *readEnd = begin + length;
  const char *p;

  if (readEnd < end) end = readEnd;
  for (p = begin; p < end && *p != '\n'; p++) continue;

  if (p != end && *p == '\n') length = p - begin + 1;

  int ret = read(buf, length);

  if (0 <= ret) buf[ret] = 0; // Terminate string

  return ret;
}
