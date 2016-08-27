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

#include "LineBuffer.h"

#include <cbang/Exception.h>
#include <cbang/os/SysError.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>

using namespace std;
using namespace cb;


void LineBuffer::write(const char *data, unsigned length) {
  while (length) {
    unsigned space = bufferSize - fill;
    unsigned bytes = space < length ? space : length;

    memcpy(buffer, data, bytes);
    length -= bytes;

    extractLines();
  }
}


void LineBuffer::read(istream &stream) {
  while (true) {
    streamsize space = bufferSize - fill;

    stream.read(buffer + fill, space);
    streamsize bytes = stream.gcount();

    if (!bytes) break;

    fill += bytes;
    extractLines();

    if (bytes < space) break;
  }
}


void LineBuffer::read(int fd) {
  while (true) {
    ssize_t space = bufferSize - fill;

    ssize_t bytes = ::read(fd, buffer + fill, space);

    if (bytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
      THROWS("Failed to read file descriptor: " << SysError());

    if (bytes <= 0) break;

    fill += bytes;
    extractLines();

    if (bytes < space) break;
  }
}


void LineBuffer::flush() {
  if (fill) {
    line(buffer, bufferSize);
    fill = 0;
  }
}


void LineBuffer::extractLines() {
  while (true) {
    const char *end = buffer + fill;
    bool done = true;

    for (const char *eol = buffer; eol < end; eol++)
      if (*eol == '\n') {
        unsigned length = eol - buffer;

        line(buffer, length);
        length++; // Skip \n

        fill -= length;
        memmove(buffer, buffer + length, fill);

        done = false;
        break;
      }

    if (done) break;
  }

  // Dump buffer if full
  if (fill == bufferSize) flush();
}
