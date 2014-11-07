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

#include "FileHandler.h"
#include "Request.h"
#include "Buffer.h"

#include <cbang/os/SystemUtilities.h>

using namespace std;
using namespace cb;
using namespace cb::Event;


FileHandler::FileHandler(const string &root, uint64_t timeout) :
  root(root), timeout(timeout), directory(SystemUtilities::isDirectory(root)) {
}


bool FileHandler::operator()(Request &req) {
  string path;

  if (directory) {
    // Relative to root
    // TODO protect against .. attacks
    path = SystemUtilities::joinPath(root, req.getURI().getPath());

  } else path = root; // Single file

  if (!SystemUtilities::isFile(path)) return false;

  // Send file
  Buffer buf;
  buf.addFile(path);
  req.reply(buf);

  if (!req.outHas("Cache-Control"))
    req.outAdd("Cache-Control", "max-age=" + String(timeout));

  return true;
}
