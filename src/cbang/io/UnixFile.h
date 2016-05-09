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

#ifndef CBANG_UNIX_FILE_H
#define CBANG_UNIX_FILE_H

#include "FileInterface.h"

namespace cb {
  class UnixFile : public FileInterface {
    int fd;

  public:
    UnixFile() : fd(-1) {}
    UnixFile(const std::string &path, std::ios::openmode mode, int perm);
    ~UnixFile() {close();}

    // From FileInterface
    void open(const std::string &path, std::ios::openmode mode, int perm);
    std::streamsize read(char *s, std::streamsize n);
    std::streamsize write(const char *s, std::streamsize n);
    std::streampos seek(std::streampos off, std::ios::seekdir way);
    bool is_open() const {return fd != -1;}
    void close();
    std::streamsize size() const;

  protected:
    virtual int _open(const std::string &path, std::ios::openmode mode,
                      int perm);
    virtual std::streamsize _read(int fd, char *s, std::streamsize n);
    virtual std::streamsize _write(int fd, const char *s, std::streamsize n);
    virtual void _close(int fd);

  public:
    static int openModeToFlags(std::ios::openmode mode);
  };
}

#endif // CBANG_UNIX_FILE_H
