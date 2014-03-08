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

#ifndef CBANG_BUFFER_DEVICE_H
#define CBANG_BUFFER_DEVICE_H

#include "Buffer.h"

#include <boost/iostreams/categories.hpp>   // bidirectional_device_tag
#include <boost/iostreams/positioning.hpp>  // stream_offset
#include <boost/iostreams/stream.hpp>


namespace cb {
  class BufferDevice {
    Buffer &buffer;

  public:
    typedef char char_type;
    typedef boost::iostreams::bidirectional_device_tag category;

    BufferDevice(Buffer &buffer) : buffer(buffer) {}

    std::streamsize read(char *s, std::streamsize n)
    {return (std::streamsize)buffer.read(s, n);}
    std::streamsize write(const char *s, std::streamsize n)
    {return (std::streamsize)buffer.write(s, n);}
  };

  typedef boost::iostreams::stream<BufferDevice> BufferStream;
}

#endif // CBANG_BUFFER_DEVICE_H

