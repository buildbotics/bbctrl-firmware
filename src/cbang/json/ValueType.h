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
#ifndef CBANG_VALUE_TYPE_H
#define CBANG_VALUE_TYPE_H

#define CBANG_ENUM_NAME ValueType
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 JSON
#define CBANG_ENUM_PATH cbang/json
#define CBANG_ENUM_PREFIX 5
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_VALUE_TYPE_H
#else // CBANG_ENUM_EXPAND

CBANG_ENUM_EXPAND(JSON_NULL,    0)
CBANG_ENUM_EXPAND(JSON_BOOLEAN, 1)
CBANG_ENUM_EXPAND(JSON_NUMBER,  2)
CBANG_ENUM_EXPAND(JSON_STRING,  3)
CBANG_ENUM_EXPAND(JSON_LIST,    4)
CBANG_ENUM_EXPAND(JSON_DICT,    5)

#endif // CBANG_ENUM_EXPAND
