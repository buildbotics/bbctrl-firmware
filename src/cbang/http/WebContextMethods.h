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
#ifndef CBANG_WEB_CONTEXT_METHODS_H
#define CBANG_WEB_CONTEXT_METHODS_H

#define CBANG_ENUM_NAME WebContextMethods
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_CASE_SENSITIVE
#define CBANG_ENUM_PATH cbang/http
#define CBANG_ENUM_PREFIX 4
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_WEB_CONTEXT_METHODS_H
#else // CBANG_ENUM_EXPAND

CBANG_ENUM_EXPAND(WCM_UNKNOWN,        0)
CBANG_ENUM_EXPAND(WCM_include,        1)
CBANG_ENUM_EXPAND(WCM_GET,            2)
CBANG_ENUM_EXPAND(WCM_REMOTE_ADDR,    3)
CBANG_ENUM_EXPAND(WCM_REQUEST_URI,    4)
CBANG_ENUM_EXPAND(WCM_REQUEST_METHOD, 5)
CBANG_ENUM_EXPAND(WCM_REQUEST_QUERY,  6)
CBANG_ENUM_EXPAND(WCM_REQUEST_PATH,   7)

#endif // CBANG_ENUM_EXPAND
