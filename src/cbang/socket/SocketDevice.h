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


namespace cb {
  class SocketDevice {
    Socket &socket;

  public:
    typedef char char_type;
    typedef boost::iostreams::bidirectional_device_tag category;

    SocketDevice(Socket &socket) : socket(socket) {}


    std::streamsize read(char *s, std::streamsize n) try {
      return (std::streamsize)socket.read(s, n);

    } catch (const Socket::EndOfStream &) {
      CBANG_LOG_DEBUG(5, "SocketDevice::read() End of stream");
      return -1;

    } catch (const Exception &e) {
      CBANG_LOG_DEBUG(5, "SocketDevice::read(): " << e);
      throw BOOST_IOSTREAMS_FAILURE(e.getMessage());
    }


    std::streamsize write(const char *s, std::streamsize n) try {
      return (std::streamsize)socket.write(s, n);

    } catch (const Exception &e) {
      CBANG_LOG_DEBUG(5, "SocketDevice::write(): " << e);
      throw BOOST_IOSTREAMS_FAILURE(e.getMessage());
    }
  };

  typedef boost::iostreams::stream<SocketDevice> SocketStream;
}
#endif // SOCKET_DEVICE_H
