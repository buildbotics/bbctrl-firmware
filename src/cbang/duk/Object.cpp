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

#include "Object.h"
#include "Array.h"
#include "Context.h"
#include "Enum.h"
#include "SmartPop.h"

#include <cbang/log/Logger.h>

#include <duktape.h>

using namespace std;
using namespace cb::duk;


Object::Object(Context &ctx, int index) :
  ctx(ctx), index(ctx.normalize(index)) {
}


bool Object::has(const string &key) {return ctx.has(index, key);}
bool Object::get(const string &key) {return ctx.get(index, key);}
bool Object::put(const string &key) {return ctx.put(index, key);}


Enum Object::enumerate(bool ownProps) {
  return ctx.enumerate(index, ownProps ? DUK_ENUM_OWN_PROPERTIES_ONLY : 0);
}


void Object::write(JSON::Sink &sink) {
  sink.beginDict();
  insertValues(sink);
  sink.endDict();
}


void Object::insertValues(JSON::Sink &sink) {
    Enum it = enumerate();
    SmartPop popEnum(ctx);

    while (it.next(true)) {
      SmartPop popKeyValue(ctx, 2);

      if (!ctx.isJSON()) continue;

      sink.beginInsert(ctx.toString(-2));
      ctx.write(-1, sink);
    }
}


Array Object::toArray(const string &key) {
  get(key);
  return Array(ctx, -1);
}


Object Object::toObject(const string &key) {
  get(key);
  return Object(ctx, -1);
}


bool Object::toBoolean(const string &key) {
  get(key);
  SmartPop pop(ctx);
  return ctx.toBoolean();
}


int Object::toInteger(const string &key) {
  get(key);
  SmartPop pop(ctx);
  return ctx.toInteger();
}


double Object::toNumber(const string &key) {
  get(key);
  SmartPop pop(ctx);
  return ctx.toNumber();
}


void *Object::toPointer(const string &key) {
  get(key);
  SmartPop pop(ctx);
  return ctx.toPointer();
}


string Object::toString(const string &key) {
  get(key);
  SmartPop pop(ctx);
  return ctx.toString();
}


void Object::setArray(const string &key) {
  ctx.pushArray();
  put(key);
}


void Object::setObject(const string &key) {
  ctx.pushObject();
  put(key);
}


void Object::setUndefined(const string &key) {
  ctx.pushUndefined();
  put(key);
}


void Object::setNull(const string &key) {
  ctx.pushNull();
  put(key);
}


void Object::setBoolean(const string &key, bool x) {
  ctx.pushBoolean(x);
  put(key);
}


void Object::setPointer(const string &key, void *x) {
  ctx.pushPointer(x);
  put(key);
}


void Object::set(const string &key, int x) {
  ctx.push(x);
  put(key);
}


void Object::set(const string &key, unsigned x) {
  ctx.push(x);
  put(key);
}


void Object::set(const string &key, double x) {
  ctx.push(x);
  put(key);
}


void Object::set(const string &key, const string &x) {
  ctx.push(x);
  put(key);
}


void Object::set(const string &key, Object &obj) {
  ctx.dup(obj.getIndex());
  put(key);
}


void Object::set(const string &key, Array &ary) {
  ctx.dup(ary.getIndex());
  put(key);
}
