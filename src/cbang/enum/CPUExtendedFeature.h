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

#ifndef CBANG_ENUM_EXPAND
#ifndef CBANG_CPU_EXTENDED_FEATURE_H
#define CBANG_CPU_EXTENDED_FEATURE_H

#define CBANG_ENUM_NAME CPUExtendedFeature
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 8
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_CPU_EXTENDED_FEATURE_H
#else // CBANG_ENUM_EXPAND

// NOTE, This is not the extended features in EAX

// ECX Register
CBANG_ENUM_EXPAND(FEATURE_SSE3,     0)
CBANG_ENUM_EXPAND(FEATURE_PCLMULDQ, 1)
CBANG_ENUM_EXPAND(FEATURE_DTES64,   2)
CBANG_ENUM_EXPAND(FEATURE_MONITOR,  3)
CBANG_ENUM_EXPAND(FEATURE_DS_CPL,   4)
CBANG_ENUM_EXPAND(FEATURE_VMX,      5)
CBANG_ENUM_EXPAND(FEATURE_SMX,      6)
CBANG_ENUM_EXPAND(FEATURE_EST,      7)
CBANG_ENUM_EXPAND(FEATURE_TM2,      8)
CBANG_ENUM_EXPAND(FEATURE_SSSE3,    9)
CBANG_ENUM_EXPAND(FEATURE_CNXT_ID,  10)
// Skip 11
CBANG_ENUM_EXPAND(FEATURE_FMA,      12)
CBANG_ENUM_EXPAND(FEATURE_CX16,     13)
CBANG_ENUM_EXPAND(FEATURE_xTPR,     14)
CBANG_ENUM_EXPAND(FEATURE_PDCM,     15)
// Skip 16
CBANG_ENUM_EXPAND(FEATURE_PCID,     17)
CBANG_ENUM_EXPAND(FEATURE_DCA,      18)
CBANG_ENUM_EXPAND(FEATURE_SSE4_1,   19)
CBANG_ENUM_EXPAND(FEATURE_SSE4_2,   20)
CBANG_ENUM_EXPAND(FEATURE_x2APIC,   21)
CBANG_ENUM_EXPAND(FEATURE_MOVBE,    22)
CBANG_ENUM_EXPAND(FEATURE_POPCNT,   23)
CBANG_ENUM_EXPAND(FEATURE_TSCD,     24)
CBANG_ENUM_EXPAND(FEATURE_AES,      25)
CBANG_ENUM_EXPAND(FEATURE_XSAVE,    26)
CBANG_ENUM_EXPAND(FEATURE_OSXSAVE,  27)

#endif // CBANG_ENUM_EXPAND
