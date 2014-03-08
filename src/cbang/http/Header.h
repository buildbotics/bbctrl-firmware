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

#ifndef CBANG_HTTP_HEADER_H
#define CBANG_HTTP_HEADER_H

#include <cbang/util/StringMap.h>

#include <string>


namespace cb {
  namespace HTTP {
    class Header : public CIStringMap {
    public:
      Header() {}
      Header(const std::string &header);

      bool hasContentType() const {return has("Content-Type");}
      const std::string &getContentType() const {return get("Content-Type");}
      void setContentType(const std::string &contentType)
      {set("Content-Type", contentType);}
      bool isContentType(const std::string &contentType) const
      {return hasContentType() && getContentType() == contentType;}
      std::string setContentTypeFromExtension(const std::string &filename);

      void setContentLength(std::streamsize length);
      std::streamsize getContentLength() const;

      void read(const std::string &data);
      std::istream &read(std::istream &stream);
      std::ostream &write(std::ostream &stream) const;

      static void parseKeyValueList(const std::string &s, StringMap &map);
      static std::string quoted(const std::string &s);
    };


    inline std::ostream &operator<<(std::ostream &stream, const Header &hdr) {
      return hdr.write(stream);
    }
  }
}

#endif // CBANG_HTTP_HEADER_H

