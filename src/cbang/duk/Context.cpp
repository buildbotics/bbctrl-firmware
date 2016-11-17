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

#include "Context.h"
#include "Object.h"
#include "Array.h"
#include "Enum.h"
#include "Module.h"
#include "SmartPop.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/log/Logger.h>
#include <cbang/json/Builder.h>
#include <cbang/debug/Debugger.h>

#include <duktape.h>

using namespace cb;
using namespace cb::duk;
using namespace std;


namespace {
  extern "C" duk_ret_t _callback(duk_context *_ctx) {
    int code = 0;
    string error;

    try {
      Context ctx(_ctx);

      // Get Callback pointer
      Object func = ctx.pushCurrentFunction();
      Callback *cb = (Callback *)func.toPointer("__callback_pointer__");
      ctx.pop();

      // Make call
      return cb->call(ctx);

    } catch (const cb::Exception &e) {
      error = SSTR(e);
      code = e.getCode();

    } catch (const std::exception &e) {
      error = e.what();
    }

    // NOTE must allow duk_internal_exceptions to pass

    duk_error(_ctx, code, error.c_str());
    return 0;
  }


  extern "C" int _get_stack_raw(duk_context *ctx) {
    if (duk_is_object(ctx, -1) && duk_has_prop_string(ctx, -1, "stack") &&
        duk_is_error(ctx, -1)) {
      duk_get_prop_string(ctx, -1, "stack");
      duk_remove(ctx, -2);
    }

    return 1;
  }
}


void duk_error_callback(duk_context *ctx) {
  duk_safe_call(ctx, _get_stack_raw, 1, 1);
}


Context::Context() : ctx(duk_create_heap_default()), deallocate(true) {
  if (!ctx) THROWS("Failed to create Duktape heap");
}


Context::~Context() {if (ctx && deallocate) duk_destroy_heap(ctx);}


int Context::type(int index) {return duk_get_type(ctx, index);}
bool Context::has(int index) {return duk_has_prop(ctx, index);}


bool Context::has(int index, int i) {
  return duk_has_prop_index(ctx, index, i);
}


bool Context::has(int index, const string &key) {
  return duk_has_prop_string(ctx, index, CPP_TO_C_STR(key));
}


bool Context::get(int index) {return duk_get_prop(ctx, index);}


bool Context::get(int index, int i) {
  return duk_get_prop_index(ctx, index, i);
}


bool Context::get(int index, const string &key) {
  return duk_get_prop_string(ctx, index, CPP_TO_C_STR(key));
}


bool Context::getGlobal(const string &key) {
  return duk_get_global_string(ctx, CPP_TO_C_STR(key));
}


bool Context::put(int index) {return duk_put_prop(ctx, index);}
bool Context::put(int index, int i) {return duk_put_prop_index(ctx, index, i);}


bool Context::put(int index, const std::string &key) {
  return duk_put_prop_string(ctx, index, CPP_TO_C_STR(key));
}


bool Context::putGlobal(const std::string &key) {
  return duk_put_global_string(ctx, CPP_TO_C_STR(key));
}


unsigned Context::top() {return (unsigned)duk_get_top(ctx);}


unsigned Context::topIndex() {
  int index = duk_get_top_index(ctx);
  if (index == DUK_INVALID_INDEX) THROWS("Stack empty");
  return (unsigned)index;
}


unsigned Context::normalize(int index) {
  return (unsigned)duk_normalize_index(ctx, index);
}


void Context::pop(unsigned n) {duk_pop_n(ctx, n);}
void Context::dup(int index) {duk_dup(ctx, index);}


void Context::compile(const string &filename) {
  push(filename);
  duk_compile(ctx, DUK_COMPILE_FUNCTION);
}


void Context::compile(const string &filename, const string &code) {
  push(code);
  compile(filename);
}


void Context::call(int nargs) {duk_call(ctx, nargs);}
void Context::callMethod(int nargs)  {duk_call_method(ctx, nargs);}
unsigned Context::length(int index) {return duk_get_length(ctx, index);}


Enum Context::enumerate(int index, int flags) {
  duk_enum(ctx, index, flags);
  return Enum(*this, -1);
}


bool Context::next(int index, bool getValue) {
  return duk_next(ctx, index, getValue);
}


void Context::write(int index, JSON::Sink &sink) {
  switch (type(index)) {
  case DUK_TYPE_UNDEFINED: break;
  case DUK_TYPE_BOOLEAN: sink.write(getBoolean(index)); break;
  case DUK_TYPE_NUMBER: sink.write(getNumber(index)); break;
  case DUK_TYPE_STRING: sink.write(getString(index)); break;
  case DUK_TYPE_NULL: sink.writeNull(); break;

  case DUK_TYPE_OBJECT:
    if (isArray(index)) getArray(index).write(sink);
    else getObject(index).write(sink);
    break;

  default: {
    dup(index);
    SmartPop pop(*this);
    sink.write(toString());
    break;
  }
  }
}


bool Context::isJSON(int index) {
  return duk_check_type_mask(ctx, index, DUK_TYPE_MASK_BOOLEAN |
                             DUK_TYPE_MASK_NUMBER | DUK_TYPE_MASK_STRING |
                             DUK_TYPE_MASK_NULL | DUK_TYPE_MASK_OBJECT);
}


bool Context::isArray(int index) {return duk_is_array(ctx, index);}
bool Context::isObject(int index) {return duk_is_object(ctx, index);}
bool Context::isBoolean(int index) {return duk_is_boolean(ctx, index);}
bool Context::isError(int index) {return duk_is_error(ctx, index);}
bool Context::isNull(int index) {return duk_is_null(ctx, index);}
bool Context::isNumber(int index) {return duk_is_number(ctx, index);}
bool Context::isPointer(int index) {return duk_is_pointer(ctx, index);}
bool Context::isString(int index) {return duk_is_string(ctx, index);}
bool Context::isUndefined(int index) {return duk_is_undefined(ctx, index);}


SmartPointer<JSON::Value> Context::toJSON(int index) {
  JSON::Builder builder;
  write(index, builder);
  return builder.getRoot();
}


Array Context::toArray(int index) {return getArray(index);}


Object Context::toObject(int index) {
  duk_to_object(ctx, index);
  return Object(*this, index);
}


bool Context::toBoolean(int index) {return duk_to_boolean(ctx, index);}
int Context::toInteger(int index) {return duk_to_int(ctx, index);}
double Context::toNumber(int index) {return duk_to_number(ctx, index);}
void *Context::toPointer(int index) {return duk_to_pointer(ctx, index);}
std::string Context::toString(int index) {return duk_to_string(ctx, index);}


Array Context::getArray(int index) {
  if (!isArray(index)) THROWS("Not an array at " << index);
  return Array(*this, index);
}


Object Context::getObject(int index) {
  if (!isObject(index)) THROWS("Not an object at " << index);
  return Object(*this, index);
}


bool Context::getBoolean(int index) {return duk_get_boolean(ctx, index);}
int Context::getInteger(int index) {return duk_get_int(ctx, index);}
double Context::getNumber(int index) {return duk_get_number(ctx, index);}
void *Context::getPointer(int index) {return duk_get_pointer(ctx, index);}


std::string Context::getString(int index) {
  return duk_get_string(ctx, index);
}


Object Context::pushGlobalObject() {
  duk_push_global_object(ctx);
  return Object(*this, -1);
}


Object Context::pushCurrentFunction() {
  duk_push_current_function(ctx);
  return Object(*this, -1);
}


Array Context::pushArray() {return Array(*this, duk_push_array(ctx));}
Object Context::pushObject() {return Object(*this, duk_push_object(ctx));}
void Context::pushUndefined() {duk_push_undefined(ctx);}
void Context::pushNull() {duk_push_null(ctx);}
void Context::pushBoolean(bool x) {duk_push_boolean(ctx, x);}
void Context::pushPointer(void *x) {duk_push_pointer(ctx, x);}
void Context::push(int x) {duk_push_int(ctx, x);}
void Context::push(unsigned x) {duk_push_uint(ctx, x);}
void Context::push(double x) {duk_push_number(ctx, x);}
void Context::push(const char *x) {duk_push_string(ctx, x);}


void Context::push(const std::string &x) {
  duk_push_lstring(ctx, CPP_TO_C_STR(x), x.length());
}



void Context::push(const SmartPointer<Callback> &cb) {
  callbacks.push_back(cb);
  duk_push_c_function(ctx, _callback, DUK_VARARGS);
  Object(*this, -1).setPointer("__callback_pointer__", cb.get());
}


void Context::defineGlobal(Module &module) {
  Object global = pushGlobalObject();
  SmartPop popGlobal(*this);
  module.define(global);
}


void Context::define(Module &module) {
  Object global = pushGlobalObject();
  SmartPop popGlobal(*this);

  Object modObject = pushObject();
  SmartPop popModObject(*this);

  module.define(modObject);
  global.set(module.getName(), modObject);
}


void Context::exec(const InputSource &source) {
  push(source.toString());
  push(source.getName());
  SmartPop pop(*this);

  if (duk_pcompile(ctx, 0)) raise("Compile failed");
  if (duk_pcall(ctx, 0)) raise("Exec failed");
}


void Context::raise(const string &msg) {
  duk_safe_call(ctx, _get_stack_raw, 1, 1);
  THROWS(msg << ": " << duk_safe_to_string(ctx, -1));
}


void Context::error(const string &msg, int code) {
  duk_error(ctx, code, msg.c_str());
}


string Context::dump() {
  duk_push_context_dump(ctx);
  SmartPop pop(*this);
  return getString();
}
