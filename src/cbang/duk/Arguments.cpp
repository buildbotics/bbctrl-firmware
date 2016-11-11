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

#include "Array.h"
#include "Object.h"
#include "Context.h"
#include "SmartPop.h"

#include <cbang/log/Logger.h>

#include <duktape.h>

using namespace cb::duk;
using namespace cb;
using namespace std;


Arguments::Arguments(Context &ctx, const Signature &sig) : ctx(ctx), sig(sig) {
  // Push missing defaults
  for (unsigned i = ctx.top(); i < sig.size(); i++)
    ctx.push(sig[i]);
}


bool Arguments::has(const string &key) {
  return !ctx.isUndefined(sig.indexOf(key));
}


Array Arguments::toArray(const string &key) {
  return Array(ctx, sig.indexOf(key));
}


Object Arguments::toObject(const string &key) {
  return Object(ctx, sig.indexOf(key));
}


bool Arguments::toBoolean(const string &key) {
  return ctx.toBoolean(sig.indexOf(key));
}


double Arguments::toNumber(const string &key) {
  return ctx.toNumber(sig.indexOf(key));
}


int Arguments::toInteger(const string &key) {
  return ctx.toInteger(sig.indexOf(key));
}


string Arguments::toString(const string &key) {
  return ctx.toString(sig.indexOf(key));
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
