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

#include "DynamicBuffer.h"

using namespace cb;
using namespace std;


void DynamicBuffer::push_front(const SmartPointer<MemoryBuffer> &buffer) {
  buffers.push_front(buffer);
  fill += buffer->getFill();
}


void DynamicBuffer::push_back(const SmartPointer<MemoryBuffer> &buffer) {
  buffers.push_back(buffer);
  fill += buffer->getFill();
}


SmartPointer<MemoryBuffer> DynamicBuffer::pop_front() {
  if (buffers.empty()) THROW("No more buffers");
  SmartPointer<MemoryBuffer> buffer = buffers.front();
  buffers.pop_front();
  fill -= buffer->getFill();
  return buffer;
}


SmartPointer<MemoryBuffer> DynamicBuffer::pop_back() {
  if (buffers.empty()) THROW("No more buffers");
  SmartPointer<MemoryBuffer> buffer = buffers.back();
  buffers.pop_back();
  fill -= buffer->getFill();
  return buffer;
}


unsigned DynamicBuffer::read(char *dst, unsigned length) {
  unsigned n = 0;

  while (fill && n < length) {
    // Get buffer
    MemoryBuffer &buf = *buffers.front();

    if (!buf.isEmpty()) {
      // Buffer -> output
      unsigned count = buf.read(dst, length - n);

      // Update counts
      n += count;
      fill -= count;
    }

    // Free empty buffers
    if (buf.isEmpty()) buffers.pop_front();
  }

  return n;
}


unsigned DynamicBuffer::write(const char *src, unsigned length) {
  unsigned n = 0;

  while (n < length) {
    // Allocate buffer
    if (buffers.empty() || buffers.back()->isFull())
      buffers.push_back(new MemoryBuffer(chunk));
    MemoryBuffer &buf = *buffers.back();

    // Input -> buffer
    unsigned count = buf.write(src, length - n);

    // Update counts
    n += count;
    fill += count;
  }

  return n;
}


unsigned DynamicBuffer::writeTo(std::ostream &stream) {
  unsigned n = 0;

  while (fill && stream) {
    // Get buffer
    MemoryBuffer &buf = *buffers.front();

    if (!buf.isEmpty()) {
      // Buffer -> stream
      unsigned count = buf.getFill();
      stream.write(buf.begin(), count);

      if (stream) {
        // Update counts
        buf.incPosition(count);
        n += count;
        fill -= count;
      }
    }

    // Free empty buffers
    if (buf.isEmpty()) buffers.pop_front();
  }

  return n;
}


unsigned DynamicBuffer::readFrom(std::istream &stream) {
  unsigned n = 0;

  while (stream) {
    // Allocate buffer
    if (buffers.empty() || buffers.back()->isFull())
      buffers.push_back(new MemoryBuffer(chunk));
    MemoryBuffer &buf = *buffers.back();

    // Stream -> buffer
    stream.read(buf.begin(), buf.getSpace());
    unsigned count = stream.gcount();

    // Update counts
    buf.incFill(count);
    n += count;
    fill += count;
  }

  return n;
}
