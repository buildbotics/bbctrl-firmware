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

#include <cbang/os/DirectoryWalker.h>

#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>

#include <cbang/Exception.h>

#include <string.h>

using namespace std;
using namespace cb;
using namespace boost;


DirectoryWalker::DirectoryWalker(const string &root, const string &pattern,
                                 unsigned maxDepth) :
  re(pattern), maxDepth(maxDepth) {

  if (root != "") init(root);
}


void DirectoryWalker::init(const string &root) {
  nextFile = "";
  dirStack.clear();
  path[0] = 0;
  push(root);
}


bool DirectoryWalker::hasNext() {
  if (!nextFile.empty()) return true;

  while (!dirStack.empty()) {
    if (!*dirStack.back()) pop();
    else {
      string name = dirStack.back()->getFilename();
      bool isDir = dirStack.back()->isSubdirectory();

      LOG_DEBUG(6, "Scanned " << name);

      dirStack.back()->next();

      if (name == ".." || name == ".") continue;

      if (isDir) {
        if (dirStack.size() < maxDepth) push(name);

      } else if (regex_match(name, re)) {
        nextFile = path + name;
        return true;
      }
    }
  }

  return false;
}


const string DirectoryWalker::next() {
  if (!hasNext()) return "";
  string tmp = nextFile;
  nextFile = "";
  return tmp;
}


void DirectoryWalker::push(const string &name) {
  string tmp = path + name;

  if (tmp[tmp.length() - 1] != '/') tmp += '/';

  dirStack.push_back(new Directory(tmp));
  path = tmp;

  LOG_DEBUG(6, "Pushed " << path);
}


void DirectoryWalker::pop() {
  string::size_type pos = path.find_last_of('/', path.length() - 2);

  if (pos == string::npos) path = "";
  else path = path.substr(0, pos + 1);

  LOG_DEBUG(6, "Popped " << path);

  dirStack.pop_back();
}
