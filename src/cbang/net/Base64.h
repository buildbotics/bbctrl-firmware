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

#ifndef CBANG_BASE64_H
#define CBANG_BASE64_H

#include <cbang/StdTypes.h>

#include <string>


namespace cb {
  class Base64 {
    const char pad;
    const char a;
    const char b;
    unsigned width;

    static const char *encodeTable;
    static const char decodeTable[256];

  public:
    Base64(char pad = '=', char a = '+', char b = '/', unsigned width = 76) :
      pad(pad), a(a), b(b), width(width) {}

    std::string encode(const std::string &s) const;
    std::string encode(const char *s, unsigned length) const;
    std::string decode(const std::string &s) const;

  protected:
    static char next(std::string::const_iterator &it,
                     std::string::const_iterator end);
    char encode(int x) const;
    int decode(char x) const;
  };
}

#endif // CBANG_BASE64_H
