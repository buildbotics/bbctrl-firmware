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

#include "MemoryBuffer.h"

#include <cbang/Exception.h>

#include <stdlib.h>
#include <string.h>

using namespace cb;
using namespace std;


MemoryBuffer::MemoryBuffer(unsigned capacity, char *buffer, bool deallocate) :
  capacity(capacity), position(0), fill(0), buffer(buffer),
  deallocate(deallocate) {

  if (buffer) fill = capacity;

  if (!buffer && capacity) {
    this->buffer = (char *)malloc(capacity);
    this->deallocate = true;
  }
}


MemoryBuffer::~MemoryBuffer() {
  if (buffer && deallocate) {
    free(buffer);
    buffer = 0;
  }
}


unsigned MemoryBuffer::read(char *dst, unsigned size) {
  if (getFill() < size) size = getFill();
  if (!size) return 0;

  memcpy(dst, begin(), size);
  incPosition(size);

  return size;
}


unsigned MemoryBuffer::write(const char *src, unsigned size) {
  if (getSpace() < size) size = getSpace();
  if (!size) return 0;

  memcpy(end(), src, size);
  incFill(size);

  return size;
}


unsigned MemoryBuffer::writeTo(ostream &stream) {
  unsigned total = 0;

  while (!isEmpty() && stream) {
    unsigned fill = getFill();
    stream.write(begin(), fill);
    if (stream) {
      incPosition(fill);
      total += fill;
    }
  }

  return total;
}


unsigned MemoryBuffer::readFrom(istream &stream) {
  unsigned total = 0;

  while (!isFull() && stream) {
    stream.read(end(), getSpace());
    unsigned count = stream.gcount();
    if (!count) break;
    incFill(count);
    total += count;
  }

  return total;
}


void MemoryBuffer::shiftToFront() {
  if (isEmpty()) clear();

  else if (position) {
    memmove(buffer, buffer + position, getFill());
    fill -= position;
    position = 0;
  }
}


unsigned MemoryBuffer::increase(unsigned size) {
  if (capacity < size) {
    if (!buffer) deallocate = true;
    if (!deallocate) THROW("Cannot increase buffer");

    unsigned newCapacity = capacity + (capacity >> 2); // At least this much
    if (newCapacity < size) newCapacity = size;

    char *newBuffer = (char *)realloc(buffer, newCapacity);
    if (!newBuffer) THROW("Allocation failed");

    capacity = newCapacity;
    buffer = newBuffer;
  }

  return capacity;
}
