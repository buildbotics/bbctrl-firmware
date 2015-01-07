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

#include "Resource.h"

#include <cbang/Exception.h>

using namespace std;
using namespace cb;


const Resource &Resource::get(const string &path) const {
  const Resource *resource = find(path);
  if (!resource) THROWS("Failed to find resource '" << path << "'");
  return *resource;
}


const Resource *DirectoryResource::find(const string &path) const {
  if (path.empty()) return 0;
  if (path[0] == '/') return find(path.substr(1));

  string::size_type pos = path.find('/');
  string name;
  if (pos == string::npos) name = path;
  else name = path.substr(0, pos);

  for (unsigned i = 0; children[i]; i++)
    if (name == children[i]->name) {
      if (pos == string::npos) return children[i];
      return children[i]->find(path.substr(pos + 1));
    }

  return 0;
}
