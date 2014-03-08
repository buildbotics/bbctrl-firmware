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
#ifndef CBANG_CPU_FEATURE_H
#define CBANG_CPU_FEATURE_H

#define CBANG_ENUM_NAME CPUFeature
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 8
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_CPU_FEATURE_H
#else // CBANG_ENUM_EXPAND

// EDX Register
CBANG_ENUM_EXPAND(FEATURE_FPU,      0)
CBANG_ENUM_EXPAND(FEATURE_VME,      1)
CBANG_ENUM_EXPAND(FEATURE_DE,       2)
CBANG_ENUM_EXPAND(FEATURE_PSE,      3)
CBANG_ENUM_EXPAND(FEATURE_TSC,      4)
CBANG_ENUM_EXPAND(FEATURE_MSR,      5)
CBANG_ENUM_EXPAND(FEATURE_PAE,      6)
CBANG_ENUM_EXPAND(FEATURE_MCE,      7)
CBANG_ENUM_EXPAND(FEATURE_CX8,      8)
CBANG_ENUM_EXPAND(FEATURE_APIC,     9)
// Skip 10
CBANG_ENUM_EXPAND(FEATURE_SEP,      11)
CBANG_ENUM_EXPAND(FEATURE_MTRR,     12)
CBANG_ENUM_EXPAND(FEATURE_PGE,      13)
CBANG_ENUM_EXPAND(FEATURE_MCA,      14)
CBANG_ENUM_EXPAND(FEATURE_CMOV,     15)
CBANG_ENUM_EXPAND(FEATURE_PAT,      16)
CBANG_ENUM_EXPAND(FEATURE_PSE_36,   17)
CBANG_ENUM_EXPAND(FEATURE_PSN,      18)
CBANG_ENUM_EXPAND(FEATURE_CLFSH,    19)
// Skip 20
CBANG_ENUM_EXPAND(FEATURE_DS,       21)
CBANG_ENUM_EXPAND(FEATURE_ACPI,     22)
CBANG_ENUM_EXPAND(FEATURE_MMX,      23)
CBANG_ENUM_EXPAND(FEATURE_FXSR,     24)
CBANG_ENUM_EXPAND(FEATURE_SSE,      25)
CBANG_ENUM_EXPAND(FEATURE_SSE2,     26)
CBANG_ENUM_EXPAND(FEATURE_SS,       27)
CBANG_ENUM_EXPAND(FEATURE_HTT,      28)
CBANG_ENUM_EXPAND(FEATURE_TM,       29)
CBANG_ENUM_EXPAND(FEATURE_IA64,     30)
CBANG_ENUM_EXPAND(FEATURE_PBE,      31)

#endif // CBANG_ENUM_EXPAND
