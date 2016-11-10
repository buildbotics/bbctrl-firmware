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

#include "Arguments.h"

#include "Signature.h"
#include "Context.h"
#include "SmartPop.h"

#include <cbang/log/Logger.h>

#include <duktape.h>

using namespace cb::duk;
using namespace cb;
using namespace std;


Arguments::Arguments(Context &ctx, const Signature &sig) :
  ctx(ctx), sig(sig), positional(ctx.top()), keyword(-1) {

  if (sig.size() && positional && ctx.isObject(-1)) {
    keyword = --positional;

    if (sig.isVariable()) return;

    // Validate keyword arguments
    duk_enum(ctx.getContext(), -1, DUK_ENUM_OWN_PROPERTIES_ONLY);
    SmartPop pop(ctx);

    while (duk_next(ctx.getContext(), -1, false)) {
      SmartPop pop(ctx);
      string name = ctx.toString();

      if (!sig.has(name))
        THROWS("Invalid keyword argument '" << name << "' when calling "
               << sig);
    }
  }
}


bool Arguments::has(unsigned index) const {
  return index < positional ||
    (0 <= keyword && ctx.hasProp(keyword, sig.keyAt(index)));
}


bool Arguments::has(const string &key) const {
  unsigned index = sig.indexOf(key);
  return index < positional || (0 <= keyword && ctx.hasProp(keyword, key));
}


bool Arguments::toBoolean(const string &key) {
  push(key);
  SmartPop pop(ctx);
  return ctx.toBoolean();
}


double Arguments::toNumber(const string &key) {
  push(key);
  SmartPop pop(ctx);
  return ctx.toNumber();
}


int Arguments::toInteger(const string &key) {
  push(key);
  SmartPop pop(ctx);
  return ctx.toInteger();
}


string Arguments::toString(const string &key) {
  push(key);
  SmartPop pop(ctx);
  return ctx.toString();
}


bool Arguments::toBoolean(unsigned index) {
  push(index);
  SmartPop pop(ctx);
  return ctx.toBoolean();
}


double Arguments::toNumber(unsigned index) {
  push(index);
  SmartPop pop(ctx);
  return ctx.toNumber();
}


int Arguments::toInteger(unsigned index) {
  push(index);
  SmartPop pop(ctx);
  return ctx.toInteger();
}


string Arguments::toString(unsigned index) {
  push(index);
  SmartPop pop(ctx);
  return ctx.toString();
}


string Arguments::toString() const {
  ostringstream str;
  str << "(" << *this << ")";
  return str.str();
}


ostream &Arguments::write(ostream &stream, const string &separator) const {
  for (int i = 0; i < ctx.top(); i++) {
    if (i) stream << separator;
    stream << ctx.toString(i);
  }

  return stream;
}


void Arguments::push(const std::string &key) {
  unsigned index = sig.indexOf(key);

  if (index < positional) ctx.dup(index);
  else if (0 <= keyword && ctx.hasProp(keyword, key)) ctx.getProp(keyword, key);
  else ctx.push(sig.get(index));
}


void Arguments::push(unsigned index) {
  if (!sig.isVariable() && sig.size() <= index)
    THROWS("Index " << index << " out of range");

  if (index < positional) ctx.dup(index);
  else {
    string key = sig.keyAt(index);
    if (0 <= keyword && ctx.hasProp(keyword, key)) ctx.getProp(keyword, key);
    else ctx.push(sig.get(index));
  }
}
