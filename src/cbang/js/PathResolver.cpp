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

#include "PathResolver.h"

#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>

using namespace cb::js;
using namespace cb;
using namespace std;


PathResolver::PathResolver() {
  pushPath(SystemUtilities::getcwd());
  addSearchExtensions("/package.json .js .json");
}


void PathResolver::pushPath(const std::string &path) {
  pathStack.push_back(path);
}


void PathResolver::popPath() {
  if (pathStack.size() == 1) THROW("No path to pop");
  pathStack.pop_back();
}


const string &PathResolver::getCurrentPath() const {
  return pathStack.back();
}


void PathResolver::addSearchExtensions(const string &exts) {
  String::tokenize(exts, searchExts);
}


void PathResolver::appendSearchExtension(const string &ext) {
  searchExts.push_back(ext);
}


string PathResolver::searchExtensions(const string &path) const {
  if (SystemUtilities::hasExtension(path) &&
      SystemUtilities::isFile(path)) return path;

  for (unsigned i = 0; i < searchExts.size(); i++) {
    string candidate = path + searchExts[i];
    LOG_DEBUG(5, "Searching " << candidate);
    if (SystemUtilities::isFile(candidate)) return candidate;
  }

  return "";
}


void PathResolver::addSearchPaths(const string &paths) {
  String::tokenize(paths, searchPaths,
                   string(1, SystemUtilities::path_delimiter));
}


string PathResolver::searchPath(const string &path) const {
  if (SystemUtilities::isAbsolute(path)) return searchExtensions(path);

  else if (String::startsWith(path, "./") || String::startsWith(path, "../")) {
    // Search relative to current file
    string candidate = SystemUtilities::absolute(getCurrentPath(), path);
    candidate = searchExtensions(candidate);
    if (SystemUtilities::isFile(candidate)) return candidate;

  } else {
    // Search paths
    for (unsigned i = 0; i < searchPaths.size(); i++) {
      string dir = searchPaths[i];
      if (!SystemUtilities::isAbsolute(dir))
        dir = SystemUtilities::absolute(getCurrentPath(), dir);

      string candidate = SystemUtilities::joinPath(dir, path);
      candidate = searchExtensions(candidate);
      if (!candidate.empty()) return candidate;
    }
  }

  return "";
}


string PathResolver::relativePath(const string &path) const {
  return SystemUtilities::absolute(getCurrentPath(), path);
}
