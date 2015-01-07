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

#ifndef CBANG_PACKET_FIELD_H
#define CBANG_PACKET_FIELD_H

#include <cbang/StdTypes.h>
#include <cbang/net/Swab.h>

#include <ostream>

namespace cb {
  /***
   * This class makes it possible to manipulate packet data fields with out
   * worrying about byte swapping and to maintain a packet inheritance
   * hierarchy that is platfrom and compiler portable.
   */
  template <typename INT_T, typename EXT_T = INT_T>
  class PacketField {
  public:
    typedef PacketField<INT_T, EXT_T> type_t;

  protected:
    INT_T &value;

  public:
    PacketField(void *data) : value(*(INT_T *)data) {}

    // Direct assignment
    const type_t &operator=(const type_t &pf) {
      value = pf.value;
      return *this;
    }

    EXT_T get() const {return (EXT_T)hton(value);}
    EXT_T operator=(EXT_T value)
    {this->value = hton((INT_T)value); return value;}
    operator EXT_T () const {return get();}
    void swab() {value = hton(value);}

    static uint16_t hton(uint16_t x) {return hton16(x);}
    static int16_t hton(int16_t x) {return hton16(x);}
    static uint32_t hton(uint32_t x) {return hton32(x);}
    static int32_t hton(int32_t x) {return hton32(x);}
    static uint64_t hton(uint64_t x) {return hton64(x);}
    static int64_t hton(int64_t x) {return hton64(x);}
    static uint128_t hton(uint128_t x) {return hton128(x);}
  };

  typedef PacketField<int16_t> PFS16;
  typedef PacketField<uint16_t> PFU16;
  typedef PacketField<int32_t> PFS32;
  typedef PacketField<uint32_t> PFU32;
  typedef PacketField<int64_t> PFS64;
  typedef PacketField<uint64_t> PFU64;
  typedef PacketField<uint128_t> PFU128;
}

#endif // CBANG_PACKET_FIELD_H
