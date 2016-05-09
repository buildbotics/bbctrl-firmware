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

#ifndef CBANG_COMPILER_INFO_H
#define CBANG_COMPILER_INFO_H

#include <cbang/SStream.h>

#ifdef __INTEL_COMPILER
#ifdef __VERSION__
#define COMPILER CBANG_SSTR(__VERSION__ << " " << __INTEL_COMPILER)

#elif _WIN32
#define COMPILER \
  CBANG_SSTR("Intel(R) C++ MSVC " << _MSC_VER << " mode " << __INTEL_COMPILER)
#else
#define COMPILER CBANG_SSTR("Intel(R) C++ " << __INTEL_COMPILER)
#endif

#elif __GNUC__
#define COMPILER "GNU " __VERSION__

#elif _MSC_VER >= 1500
#define COMPILER "Visual C++ 2008"
#elif _MSC_VER >= 1400
#define COMPILER "Visual C++ 2005"
#elif _MSC_VER >= 1310
#define COMPILER "Visual C++ 2003"
#elif _MSC_VER > 1300
#define COMPILER "Visual C++ 2002"
#else
#define COMPILER "Unknown"
#endif

#if defined(_WIN64) || defined(__x86_64__) || defined(__LP64__)
#define COMPILER_BITS 64
#else
#define COMPILER_BITS 32
#endif

#endif // CBANG_COMPILER_INFO_H
