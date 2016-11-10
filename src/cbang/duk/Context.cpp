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
#include "Arguments.h"
#include "Module.h"
#include "SmartPop.h"

#include <cbang/Exception.h>
#include <cbang/util/DefaultCatch.h>

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
      Arguments args(ctx, cb->getSignature());
      ctx.push((*cb)(args));
      return 1;

    } catch (const Exception &e) {
      error = SSTR(e);
      code = e.getCode();

    } catch (const std::exception &e) {
      error = e.what();

    } catch (...) {
      error = "unknown exception";
    }

    duk_error(_ctx, code, error.c_str());
    return 0;
  }
}


Context::Context() : ctx(duk_create_heap_default()), deallocate(true) {
  if (!ctx) THROWS("Failed to create Duktape heap");
}


Context::~Context() {if (ctx && deallocate) duk_destroy_heap(ctx);}


int Context::top() const {return duk_get_top(ctx);}
int Context::topIndex() const {return duk_get_top_index(ctx);}

void Context::pop(unsigned n) const {duk_pop_n(ctx, n);}
void Context::dup(int index) const {duk_dup(ctx, index);}

bool Context::isArray(int index) const {return duk_is_array(ctx, index);}
bool Context::isBoolean(int index) const {return duk_is_boolean(ctx, index);}
bool Context::isError(int index) const {return duk_is_error(ctx, index);}
bool Context::isNull(int index) const {return duk_is_null(ctx, index);}
bool Context::isNumber(int index) const {return duk_is_number(ctx, index);}
bool Context::isObject(int index) const {return duk_is_object(ctx, index);}
bool Context::isPointer(int index) const {return duk_is_pointer(ctx, index);}
bool Context::isString(int index) const {return duk_is_string(ctx, index);}


bool Context::isUndefined(int index) const {
  return duk_is_undefined(ctx, index);
}


bool Context::toBoolean(int index) const {return duk_to_boolean(ctx, index);}
int Context::toInteger(int index) const {return duk_to_int(ctx, index);}
double Context::toNumber(int index) const {return duk_to_number(ctx, index);}
void *Context::toPointer(int index) const {return duk_to_pointer(ctx, index);}


std::string Context::toString(int index) const {
  return duk_to_string(ctx, index);
}


Object Context::pushGlobalObject() {
  duk_push_global_object(ctx);
  return Object(*this, top() - 1);
}


Object Context::pushCurrentFunction() {
  duk_push_current_function(ctx);
  return Object(*this, top() - 1);
}


Object Context::pushObject() {return Object(*this, duk_push_object(ctx));}
void Context::pushUndefined() {duk_push_undefined(ctx);}
void Context::pushNull() {duk_push_null(ctx);}
void Context::pushBoolean(bool x) {duk_push_boolean(ctx, x);}
void Context::push(int x) {duk_push_int(ctx, x);}
void Context::push(double x) {duk_push_number(ctx, x);}
void Context::push(const char *x) {duk_push_string(ctx, x);}


void Context::push(const std::string &x) {
#if defined(_WIN32) && !defined(__MINGW32__)
  duk_push_lstring(ctx, x.c_str(), x.length());
#else
  duk_push_lstring(ctx, x.data(), x.length());
#endif
}



void Context::push(const SmartPointer<Callback> &cb) {
  callbacks.push_back(cb);
  duk_push_c_function(ctx, _callback, DUK_VARARGS);
  push("__callback_pointer__");
  duk_push_pointer(ctx, cb.get());
  putProp(-3);
}


void Context::push(const Variant &value) {
  switch (value.getType()) {
  case Variant::BOOLEAN_TYPE: pushBoolean(value.toBoolean()); break;
  case Variant::STRING_TYPE: push(value.toString()); break;
  case Variant::INTEGER_TYPE: push((int)value.toInteger()); break;
  case Variant::REAL_TYPE: push(value.toReal()); break;
  default: pushUndefined(); break;
  }
}


bool Context::hasProp(int index, const string &key) {
  push(key);
  return duk_has_prop(ctx, index) == 1;
}


bool Context::getProp(int index, const string &key) {
  push(key);
  return duk_get_prop(ctx, index) == 1;
}


void Context::putProp(int index) {duk_put_prop(ctx, index);}


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


void Context::eval(const InputSource &source) {
  push(source.toString());
  SmartPop pop(*this);
  if (duk_peval(ctx)) THROWS("Eval failed: " << duk_safe_to_string(ctx, -1));
}
