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

#include "Javascript.h"

#ifdef HAVE_V8
#include <cbang/v8/JSImpl.h>
#endif

#ifdef HAVE_CHAKRA
#include <cbang/chakra/JSImpl.h>
#endif

#include <cbang/util/SmartFunctor.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/json/JSON.h>
#include <cbang/util/SmartFunctor.h>

using namespace cb::js;
using namespace cb;
using namespace std;


Javascript::Javascript(const string &implName) : impl(0), stdMod(*this) {
#ifdef HAVE_V8
  if (implName == "v8" || (impl.isNull() && implName.empty()))
    impl = new gv8::JSImpl(*this);
#endif

#ifdef HAVE_CHAKRA
  if (implName == "chakra" || (impl.isNull() && implName.empty()))
    impl = new chakra::JSImpl(*this);
#endif

  if (impl.isNull()) {
    if (implName.empty()) THROW("No Javscript implementation compiled in");
    else THROWS("Javscript implementation '" << implName
                << "' not found in this build");
  }

  {
    SmartPointer<Scope> scope = impl->enterScope();
    nativeProps = getFactory()->createObject()->makePersistent();
  }

  define(stdMod);
  define(consoleMod);

  import("std", ".");
  import("console");
}


SmartPointer<js::Factory> Javascript::getFactory() {return impl->getFactory();}


void Javascript::define(NativeModule &mod) {
  SmartPointer<Scope> scope = impl->enterScope();

  // Create module
  SmartPointer<Module>::Phony module = &mod;
  modules.insert(modules_t::value_type(mod.getId(), module));

  try {
    SmartPointer<Value> exports = getFactory()->createObject();
    js::Sink sink(getFactory(), *exports);
    mod.define(sink);
    sink.close();
    module->setExports(exports);
  } CATCH_ERROR;
}


void Javascript::import(const string &id, const string &as) {
  SmartPointer<Scope> scope = impl->enterScope();

  // Find module
  modules_t::iterator it = modules.find(id);
  if (it == modules.end()) return THROWS("Module '" << id << "' not found");

  // Get target object
  SmartPointer<Value> target = getFactory()->createObject();
  if (as.empty()) nativeProps->set(id, *target);
  else if (as == ".") target = nativeProps;
  else nativeProps->set(as, *target);

  // Copy properties
  target->copyProperties(*it->second->getExports());
}


SmartPointer<js::Value> Javascript::eval(const InputSource &source) {
  SmartFunctor<Javascript> popPath(this, &Javascript::popPath, false);

  if (!source.getName().empty() && source.getName()[0] != '<') {
    pushPath(source.getName());
    popPath.setEngaged(true);
  }

  SmartPointer<Scope> scope = impl->enterScope();
  scope->getGlobalObject()->copyProperties(*nativeProps);

  return scope->eval(source);
}


void Javascript::interrupt() {impl->interrupt();}


SmartPointer<Value> Javascript::require(Callback &cb, Value &args) {
  string id = args.getString(0);

  modules_t::iterator it = modules.find(id);
  if (it != modules.end()) return it->second->getExports();

  string path = searchPath(id);
  if (path.empty()) THROWS("Module '" << id << "' not found");

  // Handle package.json
  if (String::endsWith(path, "/package.json")) {
    JSON::ValuePtr package = JSON::Reader(path).parse();
    path = SystemUtilities::absolute(path, package->getString("main"));
  }

  // Register module
  SmartPointer<Module> module = new Module(id, path);
  modules.insert(modules_t::value_type(id, module));

  SmartPointer<Scope> scope = impl->newScope();
  SmartPointer<Factory> factory = getFactory();
  SmartPointer<Value> exports = factory->createObject();

  if (String::endsWith(path, ".json")) {
    // Read JSON data
    js::Sink sink(factory, *exports);
    JSON::Reader(path).parse(sink);
    sink.close();

  } else {
    // Push path
    SmartFunctor<Javascript> smartPopPath(this, &Javascript::popPath);
    pushPath(path);

    // Get global object for this scope
    SmartPointer<Value> global = scope->getGlobalObject();

    // Inject native properties
    global->copyProperties(*nativeProps);

    // Create module object
    SmartPointer<Value> obj = factory->createObject();
    obj->set("id", factory->create(id));
    obj->set("exports", exports);
    obj->set("filename", factory->create(path));
    obj->set("name", factory->create(id));

    // Set global vars
    global->set("id", factory->create(id));
    global->set("exports", exports);
    global->set("module", obj);

    // Compile & eval code
    scope->eval(path);

    // Get exports, it may have been reassigned
    exports = obj->get("exports");
  }

  module->setExports(exports->makePersistent());

  return exports;
}
