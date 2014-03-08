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

#ifndef CBANG_STREAM_LIMITER_H
#define CBANG_STREAM_LIMITER_H

#include <iosfwd> // streamsize
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
#include <boost/iostreams/detail/ios.hpp>
namespace io = boost::iostreams;

#include <algorithm> // std::min()

namespace cb {
  class StreamLimiter {
   std::streamsize size;

  public:
    typedef char char_type;
    struct category : io::dual_use, io::filter_tag, io::multichar_tag {};

    StreamLimiter(std::streamsize size) : size(size) {}

    template<typename Source>
    std::streamsize read(Source &src, char *s, std::streamsize n) {
      std::streamsize count = std::min(size, n);
      if (!count) return 0;
      count = io::read(src, s, count);
      size -= count;
      return count;
    }

    template<typename Sink>
    std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
      std::streamsize count = std::min(size, n);
      if (!count) return 0;
      count = io::write(dest, s, count);
      size -= count;
      return count;
    }
  };
}

#endif // CBANG_STREAM_LIMITER_H

