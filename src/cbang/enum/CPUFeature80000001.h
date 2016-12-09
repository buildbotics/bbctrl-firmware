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
#ifndef CBANG_CPU_FEATURE_80000001_H
#define CBANG_CPU_FEATURE_80000001_H

#define CBANG_ENUM_NAME CPUFeature80000001
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 10
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_CPU_FEATURE_80000001_H
#else // CBANG_ENUM

// These are the extended features in EAX=0x80000001
// See: http://www.sandpile.org/x86/cpuid.htm

// EDX Register
CBANG_ENUM(FEATURE_X_FPU)
CBANG_ENUM(FEATURE_X_VME)
CBANG_ENUM(FEATURE_X_DE)
CBANG_ENUM(FEATURE_X_PSE)
CBANG_ENUM(FEATURE_X_TSC)
CBANG_ENUM(FEATURE_X_MSR)
CBANG_ENUM(FEATURE_X_PAE)
CBANG_ENUM(FEATURE_X_MCE)
CBANG_ENUM(FEATURE_X_CX8)
CBANG_ENUM(FEATURE_X_APIC)
CBANG_ENUM(FEATURE_X_RESERVED_EDX_10)
CBANG_ENUM(FEATURE_X_SEP)
CBANG_ENUM(FEATURE_X_MTRR)
CBANG_ENUM(FEATURE_X_PGE)
CBANG_ENUM(FEATURE_X_MCA)
CBANG_ENUM(FEATURE_X_CMOV)
CBANG_ENUM(FEATURE_X_FCMOV)
CBANG_ENUM(FEATURE_X_PSE36)
CBANG_ENUM(FEATURE_X_RESERVED_EDX_18)
CBANG_ENUM(FEATURE_X_MP)
CBANG_ENUM(FEATURE_X_NX)
CBANG_ENUM(FEATURE_X_RESERVED_EDX_21)
CBANG_ENUM(FEATURE_X_MMX_PLUS)
CBANG_ENUM(FEATURE_X_MMX)
CBANG_ENUM(FEATURE_X_FXSR)
CBANG_ENUM(FEATURE_X_FFXSR)
CBANG_ENUM(FEATURE_X_PG1G)
CBANG_ENUM(FEATURE_X_TSCP)
CBANG_ENUM(FEATURE_X_RESERVED_EDX_28)
CBANG_ENUM(FEATURE_X_LM)
CBANG_ENUM(FEATURE_X_3DNOW_PLUS)
CBANG_ENUM(FEATURE_X_3DNOW)

// ECX Register
CBANG_ENUM(FEATURE_X_AHF64)
CBANG_ENUM(FEATURE_X_CMP)
CBANG_ENUM(FEATURE_X_SVM)
CBANG_ENUM(FEATURE_X_EAS)
CBANG_ENUM(FEATURE_X_CR8D)
CBANG_ENUM(FEATURE_X_LZCNT)
CBANG_ENUM(FEATURE_X_SSE4A)
CBANG_ENUM(FEATURE_X_MSSE)
CBANG_ENUM(FEATURE_X_3DNOWP)
CBANG_ENUM(FEATURE_X_OSVW)
CBANG_ENUM(FEATURE_X_IBS)
CBANG_ENUM(FEATURE_X_XOP)
CBANG_ENUM(FEATURE_X_SKINT)
CBANG_ENUM(FEATURE_X_WDT)
CBANG_ENUM(FEATURE_X_RESERVED_ECX_14)
CBANG_ENUM(FEATURE_X_LWP)
CBANG_ENUM(FEATURE_X_FMA4)
CBANG_ENUM(FEATURE_X_TCE)
CBANG_ENUM(FEATURE_X_RESERVED_ECX_18)
CBANG_ENUM(FEATURE_X_NODEID)
CBANG_ENUM(FEATURE_X_RESERVED_ECX_20)
CBANG_ENUM(FEATURE_X_TBM)
CBANG_ENUM(FEATURE_X_TOPX)
CBANG_ENUM(FEATURE_X_PCX_CORE)
CBANG_ENUM(FEATURE_X_PCX_NB)
CBANG_ENUM(FEATURE_X_RESERVED_ECX_25)
CBANG_ENUM(FEATURE_X_DBX)
CBANG_ENUM(FEATURE_X_PERFTSC)
CBANG_ENUM(FEATURE_X_PCX_L2I)
CBANG_ENUM(FEATURE_X_MONX)
CBANG_ENUM(FEATURE_X_RESERVED_ECX_30)
CBANG_ENUM(FEATURE_X_RESERVED_ECX_31)

#endif // CBANG_ENUM
