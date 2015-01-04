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
#include "Scope.h"

#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/io/InputSource.h>
#include <cbang/util/SmartFunctor.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

using namespace cb;
using namespace cb::js;
using namespace std;


LibraryContext::LibraryContext(ostream &out) : out(out) {
  pushPath(SystemUtilities::getcwd());
  addSearchExtensions("/package.json .js .json");
}


void LibraryContext::popPath() {
  if (pathStack.size() == 1) THROW("No path top pop");
  pathStack.pop_back();
}


const string &LibraryContext::getCurrentPath() const {
  return pathStack.back();
}


void LibraryContext::addSearchExtensions(const string &exts) {
  String::tokenize(exts, searchExts);
}


void LibraryContext::appendSearchExtension(const string &ext) {
  searchExts.push_back(ext);
}


string LibraryContext::searchExtensions(const string &path) const {
  if (!SystemUtilities::extension(path).empty()) return path;

  for (unsigned i = 0; i < searchExts.size(); i++) {
    string candidate = path;
    if (!searchExts[i].empty()) candidate += searchExts[i];
    if (SystemUtilities::isFile(candidate)) return candidate;
  }

  return "";
}


void LibraryContext::addSearchPaths(const string &paths) {
  String::tokenize(paths, searchPaths,
                   string(1, SystemUtilities::path_delimiter));
}


string LibraryContext::searchPath(const string &path) const {
  if (SystemUtilities::isAbsolute(path)) {
    // Search extensions
    return searchExtensions(path);

  } else if (String::startsWith(path, "./") ||
             String::startsWith(path, "../")) {
    // Search relative to current file
    string candidate = SystemUtilities::absolute(getCurrentPath(), path);
    candidate = searchExtensions(candidate);
    if (SystemUtilities::isFile(candidate)) return candidate;

  } else {
    // Search paths
    for (unsigned i = 0; i < searchPaths.size(); i++) {
      string candidate = SystemUtilities::joinPath(searchPaths[i], path);
      candidate = searchExtensions(candidate);
      if (SystemUtilities::isFile(candidate)) return candidate;
    }
  }

  return "";
}


Value LibraryContext::require(const string &_path) {
  // TODO Check if path is a core module

  string path = searchPath(_path);

  if (path.empty()) THROWS("Module '" << _path << "' not found");

  // Normalize path
  path = SystemUtilities::getCanonicalPath(path);

  // Search already loaded modules
  modules_t::iterator it = modules.find(path);
  if (it != modules.end()) return it->second.get("exports");

  // Setup 'module.exports' for node.js style modules
  if (ctx.isNull()) ctx = new Context(*this);

  Value module = Value::createObject();
  module.set("exports", Value::createObject());
  module.set("id", path);
  module.set("filename", path);
  module.set("loaded", false);
  ctx->getGlobal().set("module", module);
  modules[path] = module;

  // Load module
  Value ret = load(path);

  module.set("loaded", true);

  Value exports = module.get("exports");
  Value names = exports.getOwnPropertyNames();

  if (!names.length()) {
    module.set("exports", ret);
    return ret;
  }

  return exports;
}


Value LibraryContext::load(const string &_path) {
  string path = _path;

  if (String::endsWith(path, "/package.json")) {
    JSON::ValuePtr package = JSON::Reader(path).parse();
    path = SystemUtilities::absolute(path, package->getString("main"));

  } else if (String::endsWith(path, ".json")) {
    if (ctx.isNull()) ctx = new Context(*this);

    Scope scope;

    Value JSON = ctx->getGlobal().get("JSON");
    Value parse = JSON.get("parse");
    Value jsonString = SystemUtilities::read(path);

    return scope.close(parse.call(JSON, jsonString));
  }

  return eval(InputSource(path));
}


Value LibraryContext::eval(const InputSource &source) {
  SmartFunctor<LibraryContext>
    smartPopPath(this, &LibraryContext::popPath, false);

  if (!source.getName().empty() && source.getName()[0] != '<') {
    pushPath(source.getName());
    smartPopPath.setEngaged(true);
  }

  if (ctx.isNull()) ctx = new Context(*this);

  return Script(*ctx, source).eval();
}
