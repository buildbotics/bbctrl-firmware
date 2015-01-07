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

#ifndef CBANG_ENUM
#ifndef CBANG_CPU_FEATURE_H
#define CBANG_CPU_FEATURE_H

#define CBANG_ENUM_NAME CPUFeature
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 8
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_CPU_FEATURE_H
#else // CBANG_ENUM

// EDX Register
CBANG_ENUM(FEATURE_FPU)
CBANG_ENUM(FEATURE_VME)
CBANG_ENUM(FEATURE_DE)
CBANG_ENUM(FEATURE_PSE)
CBANG_ENUM(FEATURE_TSC)
CBANG_ENUM(FEATURE_MSR)
CBANG_ENUM(FEATURE_PAE)
CBANG_ENUM(FEATURE_MCE)
CBANG_ENUM(FEATURE_CX8)
CBANG_ENUM(FEATURE_APIC)
CBANG_ENUM(FEATURE_RESERVED_10)
CBANG_ENUM(FEATURE_SEP)
CBANG_ENUM(FEATURE_MTRR)
CBANG_ENUM(FEATURE_PGE)
CBANG_ENUM(FEATURE_MCA)
CBANG_ENUM(FEATURE_CMOV)
CBANG_ENUM(FEATURE_PAT)
CBANG_ENUM(FEATURE_PSE_36)
CBANG_ENUM(FEATURE_PSN)
CBANG_ENUM(FEATURE_CLFSH)
CBANG_ENUM(FEATURE_RESERVED_20)
CBANG_ENUM(FEATURE_DS)
CBANG_ENUM(FEATURE_ACPI)
CBANG_ENUM(FEATURE_MMX)
CBANG_ENUM(FEATURE_FXSR)
CBANG_ENUM(FEATURE_SSE)
CBANG_ENUM(FEATURE_SSE2)
CBANG_ENUM(FEATURE_SS)
CBANG_ENUM(FEATURE_HTT)
CBANG_ENUM(FEATURE_TM)
CBANG_ENUM(FEATURE_IA64)
CBANG_ENUM(FEATURE_PBE)

#endif // CBANG_ENUM
