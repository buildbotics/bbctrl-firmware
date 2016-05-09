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

#ifndef CBANG_ENUMERATION_PACKET_FIELD_H
#define CBANG_ENUMERATION_PACKET_FIELD_H

#include "PacketField.h"
#include "EnumerationPacketField.h"

namespace cb {
  template <typename ENUM_T, typename INT_T = uint32_t>
  class EnumerationPacketField :
    public PacketField<INT_T, typename ENUM_T::enum_t> {
  public:
    typedef typename ENUM_T::enum_t enum_t;
    typedef PacketField<INT_T, enum_t> PF_T;
    typedef cb::EnumerationPacketField<ENUM_T, uint16_t> PFU16;
    typedef cb::EnumerationPacketField<ENUM_T, uint32_t> PFU32;

    EnumerationPacketField(void *data) : PF_T(data) {}

    using PF_T::operator=;
    operator ENUM_T () const {return PF_T::get();}

    const char *toString() const {return ENUM_T::toString(PF_T::get());}
    bool isValid() const {return ENUM_T::isValid(PF_T::get());}
  };


  template <typename ENUM_T>
  class EPFU16 : public EnumerationPacketField<ENUM_T, uint16_t> {
  public:
    typedef EnumerationPacketField<ENUM_T, uint16_t> Super_T;
    EPFU16(void *data) : Super_T(data) {}

    using Super_T::operator=;
  };


  template <typename ENUM_T>
  class EPFU32 : public EnumerationPacketField<ENUM_T, uint32_t> {
  public:
    typedef EnumerationPacketField<ENUM_T, uint32_t> Super_T;
    EPFU32(void *data) : Super_T(data) {}

    using Super_T::operator=;
  };

  template <typename ENUM_T, typename INT_T> inline static
  std::ostream &operator<<(std::ostream &stream,
                           const EnumerationPacketField<ENUM_T, INT_T> &pf) {
    return stream << pf.toString() << " (" << pf.get() << ")";
  }
}

#endif // CBANG_ENUMERATION_PACKET_FIELD_H
