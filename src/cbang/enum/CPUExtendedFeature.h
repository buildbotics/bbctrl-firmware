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
#ifndef CBANG_CPU_EXTENDED_FEATURE_H
#define CBANG_CPU_EXTENDED_FEATURE_H

#define CBANG_ENUM_NAME CPUExtendedFeature
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 8
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_CPU_EXTENDED_FEATURE_H
#else // CBANG_ENUM

// These are the extended features in EAX=7, ECX=0
// See: http://www.sandpile.org/x86/cpuid.htm

// EBX Register
CBANG_ENUM(FEATURE_FSGSBASE)
CBANG_ENUM(FEATURE_TSC_ADJUST)
CBANG_ENUM(FEATURE_SGX)
CBANG_ENUM(FEATURE_BMI1)
CBANG_ENUM(FEATURE_HLE)
CBANG_ENUM(FEATURE_AVX2)
CBANG_ENUM(FEATURE_FPDP)
CBANG_ENUM(FEATURE_SMEP)
CBANG_ENUM(FEATURE_BMI2)
CBANG_ENUM(FEATURE_ERMS)
CBANG_ENUM(FEATURE_INVPCID)
CBANG_ENUM(FEATURE_RTM)
CBANG_ENUM(FEATURE_PQM)
CBANG_ENUM(FEATURE_FPU_CS)
CBANG_ENUM(FEATURE_MPX)
CBANG_ENUM(FEATURE_PQE)
CBANG_ENUM(FEATURE_AVX512F)
CBANG_ENUM(FEATURE_AVX512DQ)
CBANG_ENUM(FEATURE_RDSEED)
CBANG_ENUM(FEATURE_ADX)
CBANG_ENUM(FEATURE_SMAP)
CBANG_ENUM(FEATURE_AVX512IFMA)
CBANG_ENUM(FEATURE_PCOMMIT)
CBANG_ENUM(FEATURE_CLFLUSHOPT)
CBANG_ENUM(FEATURE_CLWB)
CBANG_ENUM(FEATURE_IPT)
CBANG_ENUM(FEATURE_AVX512PF)
CBANG_ENUM(FEATURE_AVX512ER)
CBANG_ENUM(FEATURE_AVX512CD)
CBANG_ENUM(FEATURE_SHA)
CBANG_ENUM(FEATURE_AVX512BW)
CBANG_ENUM(FEATURE_AVX512VL)

// ECX Register
CBANG_ENUM(FEATURE_PREFETCHWT1)
CBANG_ENUM(FEATURE_AVX512VBMI)
CBANG_ENUM(FEATURE_UMIP)
CBANG_ENUM(FEATURE_PKU)
CBANG_ENUM(FEATURE_OSPKE)
CBANG_ENUM(FEATURE_RESERVED_ECX_5)
CBANG_ENUM(FEATURE_RESERVED_ECX_6)
CBANG_ENUM(FEATURE_CET)
CBANG_ENUM(FEATURE_RESERVED_ECX_8)
CBANG_ENUM(FEATURE_RESERVED_ECX_9)
CBANG_ENUM(FEATURE_RESERVED_ECX_10)
CBANG_ENUM(FEATURE_RESERVED_ECX_11)
CBANG_ENUM(FEATURE_RESERVED_ECX_12)
CBANG_ENUM(FEATURE_RESERVED_ECX_13)
CBANG_ENUM(FEATURE_RESERVED_ECX_14)
CBANG_ENUM(FEATURE_RESERVED_ECX_15)
CBANG_ENUM(FEATURE_VA57)
CBANG_ENUM(FEATURE_MAWAU_0)
CBANG_ENUM(FEATURE_MAWAU_1)
CBANG_ENUM(FEATURE_MAWAU_2)
CBANG_ENUM(FEATURE_MAWAU_3)
CBANG_ENUM(FEATURE_MAWAU_4)
CBANG_ENUM(FEATURE_RESERVED_ECX_23)
CBANG_ENUM(FEATURE_RESERVED_ECX_24)
CBANG_ENUM(FEATURE_RESERVED_ECX_25)
CBANG_ENUM(FEATURE_RESERVED_ECX_26)
CBANG_ENUM(FEATURE_RESERVED_ECX_27)
CBANG_ENUM(FEATURE_RESERVED_ECX_28)
CBANG_ENUM(FEATURE_RESERVED_ECX_29)
CBANG_ENUM(FEATURE_SGX_LC)
CBANG_ENUM(FEATURE_RESERVED_ECX_31)

#endif // CBANG_ENUM
