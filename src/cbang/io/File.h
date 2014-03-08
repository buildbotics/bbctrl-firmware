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

#ifndef CBANG_FILE_H
#define CBANG_FILE_H

#include "FileFactory.h"

#include <cbang/SmartPointer.h>

#include <boost/iostreams/categories.hpp> // seekable_device_tag
#include <boost/iostreams/stream.hpp>

namespace cb {
  class FileDevice {
    static SmartPointer<FileFactoryBase> factory;

    SmartPointer<FileInterface> impl;

  public:
    typedef char char_type;
    struct category :
      public io::seekable_device_tag, public io::closable_tag {};

    FileDevice(const std::string &path, std::ios::openmode mode,
               int perm = 0644);

    inline static void setFactory(const SmartPointer<FileFactoryBase> &factory)
    {FileDevice::factory = factory;}

    // From FileInterface
    std::streamsize read(char *s, std::streamsize n) {return impl->read(s, n);}
    std::streamsize write(const char *s, std::streamsize n)
    {return impl->write(s, n);}
    std::streampos seek(std::streampos off, std::ios::seekdir way)
    {return impl->seek(off, way);}
    bool is_open() const {return impl->is_open();}
    void close() {impl->close();}
    std::streamsize size() {return impl->size();}
  };


  typedef io::stream<FileDevice> File;
}

#endif // CBANG_FILE_H

