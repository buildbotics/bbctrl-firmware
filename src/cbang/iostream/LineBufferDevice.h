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

#ifndef CB_LINE_BUFFER_DEVICE_H
#define CB_LINE_BUFFER_DEVICE_H

#include <boost/iostreams/categories.hpp>   // sink_tag
#include <boost/iostreams/positioning.hpp>  // stream_offset
#include <boost/iostreams/stream.hpp>

#include <string>


namespace cb {
  template <typename T>
  class LineBufferDevice {
    T &buffer;
    std::string line;

  public:
    typedef char char_type;
    typedef boost::iostreams::sink_tag category;

    LineBufferDevice(T &buffer) : buffer(buffer) {}

    std::streamsize write(const char_type *s, std::streamsize n) {
      std::streamsize start = 0;

      for (std::streamsize i = 0; i < n; i++)
        if (s[i] == '\r' || s[i] == '\n') {
          line.append(s + start, i - start);
          buffer.append(line);
          line.clear();
          if (s[i] == '\n' && i < n - 1 && s[i + 1] == '\r') i++;
          start = i + 1;
        }

      if (start < n) line.append(s + start, n - start);

      return n;
    }
  };


  template <typename T>
  class LineBufferStream :
    public boost::iostreams::stream<LineBufferDevice<T> > {
  public:
    LineBufferStream(T &buffer) :
    boost::iostreams::stream<LineBufferDevice<T> >(buffer) {}
  };
}

#endif // CB_LINE_BUFFER_DEVICE_H
