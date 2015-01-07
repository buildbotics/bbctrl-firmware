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
#ifndef CBANG_PROCESS_PRIORITY_H
#define CBANG_PROCESS_PRIORITY_H

#define CBANG_ENUM_NAME ProcessPriority
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_PATH cbang/enum
#define CBANG_ENUM_PREFIX 9
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_PROCESS_PRIORITY_H
#else // CBANG_ENUM

CBANG_ENUM(PRIORITY_INHERIT)
CBANG_ENUM(PRIORITY_NORMAL)
CBANG_ENUM(PRIORITY_IDLE)
CBANG_ENUM(PRIORITY_LOW)
CBANG_ENUM(PRIORITY_HIGH)
CBANG_ENUM(PRIORITY_REALTIME)

#endif // CBANG_ENUM
