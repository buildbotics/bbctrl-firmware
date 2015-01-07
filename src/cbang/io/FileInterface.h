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

#ifndef CBANG_FILE_INTERFACE_H
#define CBANG_FILE_INTERFACE_H

#include <cbang/SStream.h>

#include <cbang/os/SysError.h>

#include <boost/iostreams/detail/ios.hpp>   // openmode, seekdir, int types.
#include <boost/iostreams/positioning.hpp>  // stream_offset

#define BOOST_IOS_THROWS(x) \
  throw BOOST_IOSTREAMS_FAILURE(CBANG_SSTR(x << ": " << SysError()))

namespace io = boost::iostreams;

namespace cb {
  class FileInterface {
  public:
    virtual ~FileInterface() {}

    virtual void open(const std::string &path, std::ios::openmode mode,
                      int perm) = 0;
    virtual std::streamsize read(char *s, std::streamsize n) = 0;
    virtual std::streamsize write(const char *s, std::streamsize n) = 0;
    virtual std::streampos seek(std::streampos off, std::ios::seekdir way) = 0;
    virtual bool is_open() const = 0;
    virtual void close() = 0;
    virtual std::streamsize size() const = 0;
  };
}

#endif // CBANG_FILE_INTERFACE_H

