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

#ifndef CBANG_PYON_HEADER_H
#define CBANG_PYON_HEADER_H

#include <cbang/StdTypes.h>

#include <iostream>
#include <string>

#define CBANG_PYON_MAGIC "PyON"
#define CBANG_PYON_VERSION 1


namespace cb {
  namespace PyON {
    class Header {
      const char *magic;
      uint32_t version;
      std::string type;
      bool valid;

    public:
      Header(const std::string &type = std::string(),
             const char *magic = CBANG_PYON_MAGIC,
             uint32_t version = CBANG_PYON_VERSION);

      const char *getMagic() const {return magic;}
      uint32_t getVersion() const {return version;}
      const std::string &getType() const {return type;}
      bool isValid() const {return valid;}

      void write(std::ostream &stream) const;
      void read(std::istream &stream);
    };

    static inline
    std::ostream &operator<<(std::ostream &stream, const Header &header) {
      header.write(stream); return stream;
    }

    static inline
    std::istream &operator>>(std::istream &stream, Header &header) {
      header.read(stream); return stream;
    }
  }
}

#endif // CBANG_PYON_HEADER_H
