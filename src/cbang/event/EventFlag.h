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
#ifndef CBANG_EVENT_FLAG_H
#define CBANG_EVENT_FLAG_H

#define CBANG_ENUM_NAME EventFlag
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 Event
#define CBANG_ENUM_PATH cbang/event
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_EVENT_FLAG_H
#else // CBANG_ENUM

CBANG_ENUM_VALUE(EVENT_NONE,     0)
CBANG_ENUM_VALUE(EVENT_TIMEOUT,  1 << 0)
CBANG_ENUM_VALUE(EVENT_READ,     1 << 1)
CBANG_ENUM_VALUE(EVENT_WRITE,    1 << 2)
CBANG_ENUM_VALUE(EVENT_SIGNAL,   1 << 3)
CBANG_ENUM_VALUE(EVENT_PERSIST,  1 << 4)
CBANG_ENUM_VALUE(EVENT_ET,       1 << 5)
CBANG_ENUM_VALUE(EVENT_FINALIZE, 1 << 6)
CBANG_ENUM_VALUE(EVENT_CLOSED,   1 << 7)

#endif // CBANG_ENUM
