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

#ifndef CBANG_COUNTING_STREAM_FILTER_H
#define CBANG_COUNTING_STREAM_FILTER_H

#include <iosfwd> // streamsize
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/operations.hpp>
namespace io = boost::iostreams;

namespace cb {
  class CountingStreamFilter : public io::multichar_dual_use_filter {
    unsigned count;

  public:
    struct category :
      io::multichar_dual_use_filter::category, io::flushable_tag {};

    CountingStreamFilter() : count(0) {}

    unsigned getByteCount() const {return count;}

    template<typename Source>
    std::streamsize read(Source &src, char *s, std::streamsize n) {
      n = io::read(src, s, n);
      if (n > 0) count += n;
      return n;
    }

    template<typename Sink>
    std::streamsize write(Sink &dest, const char *s, std::streamsize n) {
      n = io::write(dest, s, n);
      if (n > 0) count += n;
      return n;
    }

    template<typename Sink> bool flush(Sink &snk) {return true;}
  };
}

#endif // CBANG_Counting_STREAM_FILTER_H

