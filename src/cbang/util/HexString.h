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

#ifndef CB_HEX_STRING_H
#define CB_HEX_STRING_H

#include "Array.h"

#include <cbang/Exception.h>

#include <string>

namespace cb {
  template <unsigned SIZE>
  class HexString : public std::string {
  public:
    HexString(const std::string &s) : std::string(s) {}
    HexString(const uint8_t data[SIZE]) {
      for (unsigned i = 0; i < SIZE; i++) {
        append(1, nibbleToChar(data[i] >> 4));
        append(1, nibbleToChar(data[i]));
      }
    }


    Array<uint8_t, SIZE> parse() const {
      return parse(*this);
    }

    operator Array<uint8_t, SIZE> () const {
      return parse(*this);
    }


    inline static Array<uint8_t, SIZE> parse(const std::string &s) {
      Array<uint8_t, SIZE> array;

      for (unsigned i = 0; i < SIZE; i++)
        array[i] = (charToNibble(s.at(i * 2)) << 4) +
          charToNibble(s.at(i * 2 + 1));

      return array;
    }


    inline static char nibbleToChar(uint8_t x) {
      x &= 0xf;
      return x < 0xa ? '0' + x : 'a' + x - 0xa;
    }


    inline static uint8_t charToNibble(char c) {
      if ('0' <= c && c <= '9') return c - '0';
      if ('a' <= c && c <= 'f') return c - 'a' + 0xa;
      if ('A' <= c && c <= 'F') return c - 'A' + 0xa;
      CBANG_THROW("Invalid hex character");
    }
  };
}

#endif // CB_HEX_STRING_H

