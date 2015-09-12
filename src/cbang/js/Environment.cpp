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

#include "Environment.h"

#include "Script.h"
#include "Context.h"
#include "Scope.h"
#include "ContextScope.h"
#include "Object.h"
#include "Module.h"

#include <cbang/String.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/io/InputSource.h>
#include <cbang/util/SmartFunctor.h>
#include <cbang/json/JSON.h>

using namespace cb;
using namespace cb::js;
using namespace std;


Environment::Environment(ostream &out) : out(out) {
  pushPath(SystemUtilities::getcwd());
  addSearchExtensions("/package.json .js .json");

  // Setup global object template
  set("require(path)", this, &Environment::require);
  set("print(...)", this, &Environment::print);
  set("console", consoleMod);
}


void Environment::popPath() {
  if (pathStack.size() == 1) THROW("No path top pop");
  pathStack.pop_back();
}


const string &Environment::getCurrentPath() const {
  return pathStack.back();
}


void Environment::addSearchExtensions(const string &exts) {
  String::tokenize(exts, searchExts);
}


void Environment::appendSearchExtension(const string &ext) {
  searchExts.push_back(ext);
}


string Environment::searchExtensions(const string &path) const {
  if (SystemUtilities::hasExtension(path) &&
      SystemUtilities::isFile(path)) return path;

  for (unsigned i = 0; i < searchExts.size(); i++) {
    string candidate = path + searchExts[i];
    LOG_DEBUG(5, "Searching " << candidate);
    if (SystemUtilities::isFile(candidate)) return candidate;
  }

  return "";
}


void Environment::addSearchPaths(const string &paths) {
  String::tokenize(paths, searchPaths,
                   string(1, SystemUtilities::path_delimiter));
}


string Environment::searchPath(const string &path) const {
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


void Environment::addModule(const string &name, const Value &exports) {
  if (!modules.insert(modules_t::value_type(name, exports)).second)
    THROWS("Module '" << name << "' already added");
}


void Environment::addModule(const string &name, Module &module) {
  addModule(name, module.create());
}


Value Environment::require(const string &_path) {
  string path = searchPath(_path);

  if (path.empty()) THROWS("Module '" << _path << "' not found");

  // Normalize path
  path = SystemUtilities::getCanonicalPath(path);

  // Search already loaded modules
  modules_t::iterator it = modules.find(path);
  if (it != modules.end()) return it->second;

  // Create new context
  Scope scope;
  Context ctx(*this);
  ContextScope ctxScope(ctx);

  // Setup 'module.exports' & 'exports' for common.js style modules
  Value exports = Value::createObject();
  ctx.getGlobal().set("exports", exports);

  Value module = Value::createObject();
  module.set("exports", exports);
  module.set("id", path);
  module.set("filename", path);
  module.set("loaded", false);
  // NOTE, node.js also sets 'parent' & 'children'
  ctx.getGlobal().set("module", module);

  addModule(path, exports);

  // Load module
  Value ret = load(ctx, path);

  module.set("loaded", true);

  if (!exports.getOwnPropertyNames().length()) exports = ret;

  return scope.close(exports);
}


Value Environment::load(Context &ctx, const string &_path) {
  string path = _path;

  if (String::endsWith(path, "/package.json")) {
    JSON::ValuePtr package = JSON::Reader(path).parse();
    path = SystemUtilities::absolute(path, package->getString("main"));

  } else if (String::endsWith(path, ".json")) {
    Scope scope;

    Value JSON = ctx.getGlobal().get("JSON");
    Value parse = JSON.get("parse");
    Value jsonString = SystemUtilities::read(path);

    return scope.close(parse.call(JSON, jsonString));
  }

  return eval(ctx, InputSource(path));
}


Value Environment::load(const string &path) {
  if (ctx.isNull()) ctx = new Context(*this);
  return load(*ctx, path);
}


Value Environment::eval(Context &ctx, const InputSource &source) {
  SmartFunctor<Environment>
    smartPopPath(this, &Environment::popPath, false);

  if (!source.getName().empty() && source.getName()[0] != '<') {
    pushPath(source.getName());
    smartPopPath.setEngaged(true);
  }

  return Script(ctx, source).eval();
}


Value Environment::eval(const InputSource &source) {
  if (ctx.isNull()) ctx = new Context(*this);
  return eval(*ctx, source);
}


Value Environment::require(const Arguments &args) {
  return require(args.getString("path"));
}


void Environment::print(const Arguments &args) {
  Value JSON = Context::current().getGlobal().get("JSON");
  Value stringify = JSON.get("stringify");

  for (unsigned i = 0; i < args.getCount(); i++)
    if (args[i].isObject())
      out << stringify.call(JSON, args[i]);
    else out << args[i];

  out << flush;
}
