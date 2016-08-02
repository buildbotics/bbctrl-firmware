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

#ifndef SOCKET_DEVICE_H
#define SOCKET_DEVICE_H

#include "Socket.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#include <boost/iostreams/categories.hpp>   // bidirectional_device_tag
#include <boost/iostreams/positioning.hpp>  // stream_offset
#include <boost/iostreams/stream.hpp>

#include <vector>

#include <string.h>


namespace cb {
  class SocketDevice {
    Socket &socket;

    bool lineBuffered;
    std::vector<char> buffer;

  public:
    typedef char char_type;
    typedef boost::iostreams::bidirectional_device_tag category;

    SocketDevice(Socket &socket, bool lineBuffered = false) :
      socket(socket), lineBuffered(lineBuffered) {}


    std::streamsize read(char *s, std::streamsize n) {
      if (lineBuffered) return lineBufferedRead(s, n);
      return unbufferedRead(s, n);
    }


    std::streamsize write(const char *s, std::streamsize n) try {
      return (std::streamsize)socket.write(s, n);
    } catch (const Exception &e) {
      CBANG_LOG_DEBUG(5, "SocketDevice::write(): " << e);
      throw BOOST_IOSTREAMS_FAILURE(e.getMessage());
    }


  protected:
    std::streamsize unbufferedRead(char *s, std::streamsize n) try {
      return (std::streamsize)socket.read(s, n);

    } catch (const Exception &e) {
      if (e.getMessage() == "End of stream") return -1;
      CBANG_LOG_DEBUG(5, "SocketDevice::read(): " << e);
      throw BOOST_IOSTREAMS_FAILURE(e.getMessage());
    }


    std::streamsize lineBufferedRead(char *s, std::streamsize n) {
      if (!n) return 0;

      // Try to read some if necessary
      std::streamsize fill = buffer.size();
      if (fill < n) {
        buffer.resize(n);

        std::streamsize space = n - fill;
        std::streamsize bytes = unbufferedRead(&buffer[fill], space);

        // Handle end of stream
        if (bytes == -1) {
          if (fill) return extract(s, fill);
          return -1;
        }

        if (bytes < space) buffer.resize(fill + bytes);
      }

      // Do we have any thing?
      if (!buffer.empty()) {
        // Find last EOL
        for (std::streamsize i = buffer.size(); i; i--)
          if (buffer[i - 1] == '\n') return extract(s, i);

        // Return buffer with out EOL if requested size is less than buffer
        if (n < (std::streamsize)buffer.size()) return extract(s, n);
      }

      // Return a new line to keep std::stream happy
      *s = '\n';
      return 1;
    }


  protected:
    std::streamsize extract(char *s, std::streamsize n) {
      memcpy(s, &buffer[0], n);

      if ((std::streamsize)buffer.size() == n) buffer.resize(0);
      else {
        // Move remaining data to the front of the buffer
        std::streamsize newSize = buffer.size() - n;
        memmove(&buffer[0], &buffer[n], newSize);
        buffer.resize(newSize);
      }

      return n;
    }
  };

  typedef boost::iostreams::stream<SocketDevice> SocketStream;
}
#endif // SOCKET_DEVICE_H
