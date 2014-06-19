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

#include "LibraryContext.h"

#include "Script.h"
#include "Context.h"

#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/io/InputSource.h>

using namespace cb;
using namespace cb::js;
using namespace std;


Value LibraryContext::load(const string &_path) {
  string path = _path;

  // TODO Check if path is a core module

  // Check absolute/relative paths
  if (String::startsWith(path, "./") || String::startsWith(path, "/") ||
      String::startsWith(path, "../")) {

    if (!String::startsWith(path, "/") && !current.empty())
      path = SystemUtilities::joinPath
        (SystemUtilities::dirname(current.back()), path);

    path = SystemUtilities::absolute(path);

  } else {
    // TODO Search paths
  }

  // Search already loaded modules
  modules_t::iterator it = modules.find(path);
  if (it != modules.end()) return it->second;

  ObjectTemplate tmpl;
  Context ctx(tmpl);
  Script script(ctx, InputSource(path));
  script.eval();

  Value module = ctx.getGlobal();
  modules[path] = module;

  return module;
}
