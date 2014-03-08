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

#ifndef CBANG_TAR_HEADER_T
#define CBANG_TAR_HEADER_T

#include <cbang/StdTypes.h>
#include <string>
#include <iostream>

namespace cb {
  struct TarHeader {
    enum type_t {
      NORMAL_FILE,
      HARD_LINK,
      SYMBOLIC_LINK,
      CHARACTER_DEV,
      BLOCK_DEV,
      DIRECTORY,
      FIFO,
      CONTIGUOUS_FILE,
    };

    char filename[100];
    char mode[8];
    char owner[8];
    char group[8];
    char size[12];
    char mod_time[12];
    char checksum[8];
    char type[1];
    char link_name[100];
    char reserved[255];

    bool checksum_valid;

    TarHeader(const std::string &filename = "", uint64_t size = 0);

    unsigned updateChecksum();

    bool read(std::istream &stream);
    void write(std::ostream &stream);

    void setFilename(const std::string &filename);
    void setMode(uint32_t mode);
    void setOwner(uint32_t owner);
    void setGroup(uint32_t group);
    void setSize(uint64_t size);
    void setModTime(uint64_t mod_time);
    void setType(type_t type);
    void setLinkName(const std::string &link_name);

    const std::string getFilename() const;
    uint32_t getMode() const;
    uint32_t getOwner() const;
    uint32_t getGroup() const;
    uint64_t getSize() const;
    uint64_t getModTime() const;
    type_t getType() const;
    const std::string getLinkName() const;

    bool isEOF() const;

    static void writeNumber(uint64_t n, char *buf, uint32_t length);
    static void writeString(const std::string &s, char *buf, uint32_t length);
    static uint64_t readNumber(const char *buf, uint32_t length);

  protected:
    unsigned computeChecksum();
  };
}

#endif // CBANG_TAR_HEADER_T
