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

#ifndef SWAB_H
#define SWAB_H

#include <cbang/StdTypes.h>

#ifdef _WIN32
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN    4321
#define BYTE_ORDER    LITTLE_ENDIAN

#elif __APPLE__
#include <machine/endian.h>

#else // POSIX systems
#include <endian.h>
#endif

inline uint16_t swap16(uint16_t x) {return (x << 8) | (x >> 8);}
inline int16_t swap16(int16_t x) {return (int16_t)swap16((uint16_t)x);}
inline uint32_t swap32(uint32_t x)
{return ((uint32_t)swap16((uint16_t)x) << 16) | swap16((uint16_t)(x >> 16));}
inline int32_t swap32(int32_t x) {return (int32_t)swap32((uint32_t)x);}
inline uint64_t swap64(uint64_t x)
{return ((uint64_t)swap32((uint32_t)x) << 32) | swap32((uint32_t)(x >> 32));}
inline int64_t swap64(int64_t x) {return (int64_t)swap64((uint64_t)x);}
// TODO Is this correct?  Shouldn't we also swap the 64-bit parts?
// Bug inherited from Cosm?
inline uint128_t swap128(uint128_t x) {return uint128_t(x.lo, x.hi);}

#if BYTE_ORDER == LITTLE_ENDIAN
#define hton16(x) swap16(x)
#define hton32(x) swap32(x)
#define hton64(x) swap64(x)
#define hton128(x) swap128(x)

#define htol16(x) (x)
#define htol32(x) (x)
#define htol64(x) (x)
#define htol128(x) (x)

#else // BIG_ENDIAN
#define hton16(x) (x)
#define hton32(x) (x)
#define hton64(x) (x)
#define hton128(x) (x)

#define htol16(x) swap16(x)
#define htol32(x) swap32(x)
#define htol64(x) swap64(x)
#define htol128(x) swap128(x)
#endif

#endif // SWAB_H
