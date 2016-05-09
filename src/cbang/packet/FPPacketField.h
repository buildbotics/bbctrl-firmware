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

#ifndef CBANG_FPPACKET_FIELD_H
#define CBANG_FPPACKET_FIELD_H

#include "PacketField.h"

#include <cbang/String.h>
#include <cbang/util/SaveOStreamConfig.h>

#include <ostream>
#include <iomanip>

namespace cb {
  /***
   * This class implements a floating point value stored as byte swapped fixed
   * point packet field.
   */
  template <typename INT_T, unsigned MULTIPLIER, typename EXT_T>
  class FPPacketField : public PacketField<INT_T, EXT_T> {
  public:
    typedef FPPacketField<INT_T, MULTIPLIER, EXT_T> type_t;
    typedef PacketField<INT_T, EXT_T> Super;

    FPPacketField(void *data) : Super(data) {}

    int getPrecision() const {
      int precision;
      unsigned x = MULTIPLIER;
      for (precision = 0; 1 < x; precision++) x /= 10;
      return precision;
    }

    // Direct assignment
    const type_t &operator=(const type_t &pf) {
      Super::value = pf.value;
      return *this;
    }

    EXT_T get() const {return (EXT_T)this->hton(Super::value) / MULTIPLIER;}
    EXT_T operator=(EXT_T value) {
      this->Super::value = this->hton((INT_T)(value * MULTIPLIER));
      return value;
    }
    operator EXT_T () const {return get();}
  };


  template <typename INT_T, unsigned MULTIPLIER, typename EXT_T>
  std::ostream &operator<<(std::ostream &stream,
                           const FPPacketField<INT_T, MULTIPLIER, EXT_T> &pf) {
    SaveOStreamConfig save(stream);
    return
      stream << std::setprecision(pf.getPrecision()) << std::fixed << pf.get();
  }

  typedef FPPacketField<int16_t,  100, float>  PFS16F2;
  typedef FPPacketField<uint16_t, 100, float>  PFU16F2;
  typedef FPPacketField<int32_t,  100, double> PFS32F2;
  typedef FPPacketField<uint32_t, 100, double> PFU32F2;
  typedef FPPacketField<int64_t,  100, double> PFS64F2;
  typedef FPPacketField<uint64_t, 100, double> PFU64F2;

  typedef FPPacketField<int16_t,  1000, float>  PFS16F3;
  typedef FPPacketField<uint16_t, 1000, float>  PFU16F3;
  typedef FPPacketField<int32_t,  1000, double> PFS32F3;
  typedef FPPacketField<uint32_t, 1000, double> PFU32F3;
  typedef FPPacketField<int64_t,  1000, double> PFS64F3;
  typedef FPPacketField<uint64_t, 1000, double> PFU64F3;
}

#endif // CBANG_FPPACKET_FIELD_H
