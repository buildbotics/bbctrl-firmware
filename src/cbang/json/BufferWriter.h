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

#ifndef CB_JSON_BUFFER_WRITER_H
#define CB_JSON_BUFFER_WRITER_H

#include "Writer.h"

#include <cbang/iostream/VectorDevice.h>

#include <vector>

namespace cb {
  namespace JSON {
    class BufferWriter : public Writer {
      std::vector<char> buffer;
      VectorStream<char> stream;

    public:
      BufferWriter(unsigned indent = 0, bool compact = false,
                   output_mode_t mode = Writer::JSON_MODE) :
        Writer(stream, indent, compact, mode), stream(buffer) {}

#ifdef _WIN32
      const char *data() const {return &buffer[0];}
#else
      const char *data() const {return buffer.data();}
#endif

      size_t const size() const {return buffer.size();}
      std::string toString() const {return std::string(data(), size());}

      void flush() {stream.flush();}
    };
  }
}

#endif // CB_JSON_BUFFER_WRITER_H

