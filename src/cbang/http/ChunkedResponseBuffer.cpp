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

#include "ChunkedResponseBuffer.h"

#include <cbang/String.h>

#include <stdio.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


ChunkedResponseBuffer::
ChunkedResponseBuffer(const SmartPointer<Buffer> &source) :
  source(source), sizePtr(0), fill(0), ptr(0) {
}


unsigned ChunkedResponseBuffer::read(char *dst, unsigned length) {
  if (ptr == fill) {
    fill = source->read(chunk, 4096 - 2);
    ptr = 0;

    if (fill) {
      sizePtr = size;
      sprintf(size, "%x\r\n", fill);
      chunk[fill++] = '\r';
      chunk[fill++] = '\n';
    }
  }

  unsigned count = 0;

  // Copy chunk size
  while (count < length && sizePtr && *sizePtr) {
    *dst++ = *sizePtr++;
    count++;
  }

  // Copy chunk
  while (count < length && ptr < fill) {
    *dst++ = chunk[ptr++];
    count++;
  }

  return count;
}
