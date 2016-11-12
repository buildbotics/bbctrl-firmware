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

#include "Array.h"
#include "Object.h"
#include "Context.h"
#include "SmartPop.h"

#include <cbang/log/Logger.h>

#include <duktape.h>

using namespace std;
using namespace cb::duk;


bool Array::has(int i) const {
  return duk_has_prop_index(ctx.getContext(), index, i);
}


bool Array::get(int i) const {
  return duk_get_prop_index(ctx.getContext(), index, i);
}


bool Array::put(int i) {
  return duk_put_prop_index(ctx.getContext(), index, i);
}


unsigned Array::length() const {
  return duk_get_length(ctx.getContext(), index);
}


int Array::getType(int i) const {
  get(i);
  SmartPop pop(ctx);
  return ctx.getType();
}


bool Array::isArray(int i) const {
  get(i);
  SmartPop pop(ctx);
  return ctx.isArray();
}


bool Array::isObject(int i) const {return getType(i) == DUK_TYPE_OBJECT;}
bool Array::isBoolean(int i) const {return getType(i) == DUK_TYPE_BOOLEAN;}


bool Array::isError(int i) const {
  get(i);
  SmartPop pop(ctx);
  return ctx.isError();
}


bool Array::isNull(int i) const {return getType(i) == DUK_TYPE_NULL;}
bool Array::isNumber(int i) const {return getType(i) == DUK_TYPE_NUMBER;}
bool Array::isPointer(int i) const {return getType(i) == DUK_TYPE_POINTER;}
bool Array::isString(int i) const {return getType(i) == DUK_TYPE_STRING;}
bool Array::isUndefined(int i) const {return getType(i) == DUK_TYPE_UNDEFINED;}


Array Array::toArray(int i) {
  get(i);
  return Array(ctx, ctx.top() - 1);
}


Object Array::toObject(int i) {
  get(i);
  return Object(ctx, ctx.top() - 1);
}


bool Array::toBoolean(int i) {
  get(i);
  SmartPop pop(ctx);
  return ctx.toBoolean();
}


int Array::toInteger(int i) {
  get(i);
  SmartPop pop(ctx);
  return ctx.toInteger();
}


double Array::toNumber(int i) {
  get(i);
  SmartPop pop(ctx);
  return ctx.toNumber();
}


void *Array::toPointer(int i) {
  get(i);
  SmartPop pop(ctx);
  return ctx.toPointer();
}


string Array::toString(int i) {
  get(i);
  SmartPop pop(ctx);
  return ctx.toString();
}


void Array::setNull(int i) {
  ctx.pushNull();
  put(i);
}


void Array::setBoolean(int i, bool x) {
  ctx.pushBoolean(x);
  put(i);
}


void Array::set(int i, int x) {
  ctx.push(x);
  put(i);
}


void Array::set(int i, unsigned x) {
  ctx.push(x);
  put(i);
}


void Array::set(int i, double x) {
  ctx.push(x);
  put(i);
}


void Array::set(int i, const string &x) {
  ctx.push(x);
  put(i);
}


void Array::set(int i, Object &obj) {
  ctx.dup(obj.getIndex());
  put(i);
}


void Array::set(int i, Array &ary) {
  ctx.dup(ary.getIndex());
  put(i);
}
