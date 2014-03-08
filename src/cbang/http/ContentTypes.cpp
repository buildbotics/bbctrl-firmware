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

#include "ContentTypes.h"

using namespace std;
using namespace cb;
using namespace cb::HTTP;

SINGLETON_DECL(ContentTypes);


ContentTypes::ContentTypes(Inaccessible) {
  insert(value_type("png",  "image/png"));
  insert(value_type("ico",  "image/x-icon"));
  insert(value_type("css",  "text/css"));
  insert(value_type("txt",  "text/plain"));
  insert(value_type("c",    "text/plain"));
  insert(value_type("cpp",  "text/plain"));
  insert(value_type("c++",  "text/plain"));
  insert(value_type("h",    "text/plain"));
  insert(value_type("hpp",  "text/plain"));
  insert(value_type("py",   "text/plain"));
  insert(value_type("xml",  "text/xml"));
  insert(value_type("html", "text/html"));
  insert(value_type("htm",  "text/html"));
  insert(value_type("js",   "text/javascript"));
  insert(value_type("json", "application/json"));
  insert(value_type("tar",  "application/x-tar"));
  insert(value_type("bz2",  "application/x-bzip2"));
  insert(value_type("gz",   "application/x-gzip"));
  insert(value_type("crt",  "application/x-x509-ca-cert"));
}
