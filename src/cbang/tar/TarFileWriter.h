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

#ifndef CB_TAR_FILE_WRITER_H
#define CB_TAR_FILE_WRITER_H

#include "TarFile.h"

#include <cbang/SmartPointer.h>
#include <cbang/StdTypes.h>

#include <ostream>
#include <string>


namespace cb {
  class TarFileWriter : public TarFile {
    struct private_t;
    private_t *pri;
    SmartPointer<std::ostream> stream;

  public:
    TarFileWriter(const std::string &path, std::ios::openmode mode,
                  int perm = 0644, compression_t compression = TARFILE_AUTO);
    TarFileWriter(std::ostream &stream, compression_t compression);
    ~TarFileWriter();

    void add(const std::string &path,
             const std::string &filename = std::string(), uint32_t mode = 0);
    void add(std::istream &in, const std::string &filename, uint64_t size,
             uint32_t mode = 0644);
    void add(const char *data, size_t size, const std::string &filename,
             uint32_t mode = 0644);

    using Tar::writeHeader;

  protected:
    void writeHeader(type_t type, const std::string &filename, uint64_t size,
                     uint32_t mode);
    void addCompression(compression_t compression);
  };
}

#endif // CB_TAR_FILE_WRITER_H
