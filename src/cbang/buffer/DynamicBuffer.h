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

#ifndef CB_DYNAMIC_BUFFER_H
#define CB_DYNAMIC_BUFFER_H

#include "MemoryBuffer.h"
#include "BufferDevice.h"

#include <cbang/SmartPointer.h>

#include <list>


namespace cb {
  class DynamicBuffer : public Buffer, public BufferDevice {
    const unsigned chunk;
    unsigned fill;

    typedef std::list<SmartPointer<MemoryBuffer> > buffers_t;
    buffers_t buffers;

  public:
    DynamicBuffer(unsigned chunk = 4096) :
      BufferDevice(*static_cast<Buffer *>(this)), chunk(chunk) {}

    typedef buffers_t::const_iterator iterator;
    iterator begin() const {return buffers.begin();}
    iterator end() const {return buffers.end();}

    void clear() {buffers.clear(); fill = 0;}

    bool hasBuffer() const {return !buffers.empty();}
    void push_front(const SmartPointer<MemoryBuffer> &buffer);
    void push_back(const SmartPointer<MemoryBuffer> &buffer);
    SmartPointer<MemoryBuffer> pop_front();
    SmartPointer<MemoryBuffer> pop_back();

    // From Buffer
    unsigned getFill() const {return fill;}
    unsigned getSpace() const {return ~0;}

    unsigned read(char *dst, unsigned length);
    unsigned write(const char *src, unsigned length);
    unsigned writeTo(std::ostream &stream);
    unsigned readFrom(std::istream &stream);
  };

  typedef boost::iostreams::stream<DynamicBuffer> DynamicBufferStream;
}

#endif // CB_DYNAMIC_BUFFER_H
