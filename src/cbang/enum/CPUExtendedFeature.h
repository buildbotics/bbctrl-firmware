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

#ifndef CBANG_ENUM
#ifndef CBANG_CPU_EXTENDED_FEATURE_H
#define CBANG_CPU_EXTENDED_FEATURE_H

#define CBANG_ENUM_NAME CPUExtendedFeature
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 8
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_CPU_EXTENDED_FEATURE_H
#else // CBANG_ENUM

// NOTE, This is not the extended features in EAX

// ECX Register
CBANG_ENUM(FEATURE_SSE3)
CBANG_ENUM(FEATURE_PCLMULDQ)
CBANG_ENUM(FEATURE_DTES64)
CBANG_ENUM(FEATURE_MONITOR)
CBANG_ENUM(FEATURE_DS_CPL)
CBANG_ENUM(FEATURE_VMX)
CBANG_ENUM(FEATURE_SMX)
CBANG_ENUM(FEATURE_EST)
CBANG_ENUM(FEATURE_TM2)
CBANG_ENUM(FEATURE_SSSE3)
CBANG_ENUM(FEATURE_CNXT_ID)
CBANG_ENUM(FEATURE_RESERVED_11)
CBANG_ENUM(FEATURE_FMA)
CBANG_ENUM(FEATURE_CX16)
CBANG_ENUM(FEATURE_xTPR)
CBANG_ENUM(FEATURE_PDCM)
CBANG_ENUM(FEATURE_RESERVED_16)
CBANG_ENUM(FEATURE_PCID)
CBANG_ENUM(FEATURE_DCA)
CBANG_ENUM(FEATURE_SSE4_1)
CBANG_ENUM(FEATURE_SSE4_2)
CBANG_ENUM(FEATURE_x2APIC)
CBANG_ENUM(FEATURE_MOVBE)
CBANG_ENUM(FEATURE_POPCNT)
CBANG_ENUM(FEATURE_TSCD)
CBANG_ENUM(FEATURE_AES)
CBANG_ENUM(FEATURE_XSAVE)
CBANG_ENUM(FEATURE_OSXSAVE)

#endif // CBANG_ENUM
