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
#ifndef CBANG_STATUS_CODE_H
#define CBANG_STATUS_CODE_H

#define CBANG_ENUM_NAME StatusCode
#define CBANG_ENUM_NAMESPACE cb
#define CBANG_ENUM_NAMESPACE2 HTTP
#define CBANG_ENUM_PATH cbang/http
#include <cbang/enum/MakeEnumeration.def>

#endif // CBANG_STATUS_CODE_H
#else // CBANG_ENUM_EXPAND

CBANG_ENUM_EXPAND(HTTP_UNKNOWN, 0)
CBANG_ENUM_EXPAND(HTTP_CONTINUE, 100)
CBANG_ENUM_EXPAND(HTTP_SWITCHING_PROTOCOLS, 101)
CBANG_ENUM_EXPAND(HTTP_OK, 200)
CBANG_ENUM_EXPAND(HTTP_CREATED, 201)
CBANG_ENUM_EXPAND(HTTP_ACCEPTED, 202)
CBANG_ENUM_EXPAND(HTTP_NON_AUTHORITATIVE_INFORMATION, 203)
CBANG_ENUM_EXPAND(HTTP_NO_CONTENT, 204)
CBANG_ENUM_EXPAND(HTTP_RESET_CONTENT, 205)
CBANG_ENUM_EXPAND(HTTP_PARTIAL_CONTENT, 206)
CBANG_ENUM_EXPAND(HTTP_MULTIPLE_CHOICES, 300)
CBANG_ENUM_EXPAND(HTTP_MOVED_PERMANENTLY, 301)
CBANG_ENUM_EXPAND(HTTP_FOUND, 302)
CBANG_ENUM_EXPAND(HTTP_SEE_OTHER, 303)
CBANG_ENUM_EXPAND(HTTP_NOT_MODIFIED, 304)
CBANG_ENUM_EXPAND(HTTP_USE_PROXY, 305)
CBANG_ENUM_EXPAND(HTTP_TEMPORARY_REDIRECT, 307)
CBANG_ENUM_EXPAND(HTTP_BAD_REQUEST, 400)
CBANG_ENUM_EXPAND(HTTP_UNAUTHORIZED, 401)
CBANG_ENUM_EXPAND(HTTP_PAYMENT_REQUIRED, 402)
CBANG_ENUM_EXPAND(HTTP_FORBIDDEN, 403)
CBANG_ENUM_EXPAND(HTTP_NOT_FOUND, 404)
CBANG_ENUM_EXPAND(HTTP_METHOD_NOT_ALLOWED, 405)
CBANG_ENUM_EXPAND(HTTP_NOT_ACCEPTABLE, 406)
CBANG_ENUM_EXPAND(HTTP_PROXY_AUTHENTICATION_REQUIRED, 407)
CBANG_ENUM_EXPAND(HTTP_REQUEST_TIME_OUT, 408)
CBANG_ENUM_EXPAND(HTTP_CONFLICT, 409)
CBANG_ENUM_EXPAND(HTTP_GONE, 410)
CBANG_ENUM_EXPAND(HTTP_LENGTH_REQUIRED, 411)
CBANG_ENUM_EXPAND(HTTP_PRECONDITION_FAILED, 412)
CBANG_ENUM_EXPAND(HTTP_REQUEST_ENTITY_TOO_LARGE, 413)
CBANG_ENUM_EXPAND(HTTP_REQUEST_URI_TOO_LARGE, 414)
CBANG_ENUM_EXPAND(HTTP_UNSUPPORTED_MEDIA_TYPE, 415)
CBANG_ENUM_EXPAND(HTTP_REQUESTED_RANGE_NOT_SATISFIABLE, 416)
CBANG_ENUM_EXPAND(HTTP_EXPECTATION_FAILED, 417)
CBANG_ENUM_EXPAND(HTTP_INTERNAL_SERVER_ERROR, 500)
CBANG_ENUM_EXPAND(HTTP_NOT_IMPLEMENTED, 501)
CBANG_ENUM_EXPAND(HTTP_BAD_GATEWAY, 502)
CBANG_ENUM_EXPAND(HTTP_SERVICE_UNAVAILABLE, 503)
CBANG_ENUM_EXPAND(HTTP_GATEWAY_TIME_OUT, 504)
CBANG_ENUM_EXPAND(HTTP_VERSION_NOT_SUPPORTED, 505)

#endif // CBANG_ENUM_EXPAND
