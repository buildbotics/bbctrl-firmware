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

#include "JSImpl.h"
#include "Value.h"
#include "Sink.h"
#include "Module.h"

#include <cbang/js/Javascript.h>
#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/util/SmartFunctor.h>
#include <cbang/json/JSON.h>
#include <cbang/os/SystemUtilities.h>

using namespace cb::chakra;
using namespace cb;
using namespace std;


namespace {
  extern "C" JsValueRef _require(JsValueRef callee, bool constructor,
                                 JsValueRef *argv, unsigned short argc,
                                 void *data) {
    try {
      if (argc != 2) THROW("Invalid arguments");

      return ((JSImpl *)data)->require(Value(argv[1]).toString());

    } catch (const Exception &e) {
      JsSetException(Value::createError(e.getMessage()));
    }

    return Value::getUndefined();
  }


  extern "C" void _collect(void *data) {LOG_INFO(1, "GC");}
  extern "C" bool _alloc(void *data, JsMemoryEventType event, size_t size) {
    LOG_INFO(1, "Alloc(" << event << ", " << size << ")");
    return true;
  }
}


JSImpl::JSImpl(js::Javascript &js) : js(js) {
  CHAKRA_CHECK(JsCreateRuntime(JsRuntimeAttributeNone, 0, &runtime));
  //JsSetRuntimeBeforeCollectCallback(runtime, 0, _collect);
  //JsSetRuntimeMemoryAllocationCallback(runtime, 0, _alloc);

  ctx = new Context(*this);
  common = new ValueRef(Value::createObject());

  common->set("require", Value("require", _require, this));
}


JSImpl::~JSImpl() {
  common.release();
  modules.clear();
  ctx.release();
  JsSetCurrentContext(JS_INVALID_REFERENCE);
  JsDisposeRuntime(runtime);
}


JSImpl &JSImpl::current() {return Context::current().getImpl();}


Value JSImpl::require(const string &id) {
  modules_t::iterator it = modules.find(id);
  if (it != modules.end()) return it->second->getObject().get("exports");

  string path = js.searchPath(id);
  if (path.empty()) THROWS("Module '" << id << "' not found");

  // Handle package.json
  if (String::endsWith(path, "/package.json")) {
    JSON::ValuePtr package = JSON::Reader(path).parse();
    path = SystemUtilities::absolute(path, package->getString("main"));
  }

  // Register module
  SmartPointer<Module> module = new Module(id, path, *this);
  modules.insert(modules_t::value_type(id, module));

  if (String::endsWith(path, ".json")) {
    // Read JSON data
    Value exports = module->getObject().get("exports");
    Sink sink(exports);
    JSON::Reader(path).parse(sink);
    sink.close();
    return exports;

  } else {
    // Push path
    SmartFunctor<js::Javascript>smartPopPath(&js, &js::Javascript::popPath);
    js.pushPath(path);

    // Read Javscript code
    string code = SystemUtilities::read(path);

    // Inject common properties
    Value::getGlobal().copyProperties(*common);

    // Compile & eval
    return module->load(code);
  }
}


void JSImpl::enable() {JsEnableRuntimeExecution(runtime);}


void JSImpl::define(js::Module &mod) {
  SmartPointer<Module> module = new Module(mod.getName(), "<native>", *this);
  modules.insert(modules_t::value_type(mod.getName(), module));

  try {
    Sink sink(module->getObject().get("exports"));
    mod.define(sink);
    sink.close();
  } CATCH_ERROR;
}


void JSImpl::import(const string &module, const string &as) {
  modules_t::iterator it = modules.find(module);
  if (it == modules.end()) return THROWS("Module '" << module << "' not found");

  // Get target object
  ctx->enter();
  Value target;
  if (as.empty()) target = common->setObject(module);
  else if (as == ".") target = *common;
  else target = common->setObject(as);

  // Copy properties
  target.copyProperties(it->second->getObject().get("exports"));
}


void JSImpl::exec(const InputSource &source) {
  // Inject common properties
  ctx->enter();
  Value::getGlobal().copyProperties(*common);

  // Execute script
  ctx->exec(source.getName(), source.toString());

  // Check for errors
  if (Value::hasException()) {
    Value ex = Value::getException();
    ostringstream msg;

    if (ex.isObject() && ex.has("stack")) msg << ex.getString("stack");
    else if (ex.isObject() && ex.has("line"))
      msg << ex.toString() << "\n  at " << source.getName() << ':'
          << ex.getInteger("line") << ':'
          << ex.getInteger("column");
    else msg << ex.toString();

    THROW(msg.str());
  }
}


void JSImpl::interrupt() {JsDisableRuntimeExecution(runtime);}
