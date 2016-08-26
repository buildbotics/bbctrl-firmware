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

#include "MemberFunctor.h"
#include "StdLibrary.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <stdarg.h>
#include <ctype.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::Script;


Environment::Environment(const Environment &env) :
  map<string, SmartPointer<Entity> >(env), parent(env.parent), name(env.name) {}


Environment::Environment(const string &name, Handler *parent) :
  parent(parent ? parent : &StdLibrary::instance()), name(name) {
  typedef MemberFunctor<Environment> MF;
  add(new MF("help", this, &Environment::evalHelp, 0, 1));
  add(new MF("set", this, &Environment::evalSet, 2, 2));
  add(new MF("unset", this, &Environment::evalUnset, 1, 1));
}


const SmartPointer<Entity> &Environment::add(const SmartPointer<Entity> &e) {
  if (!insert(value_type(e->getName(), e)).second)
    THROWS("Environment already has '" << e->getName() << "'");

  return e;
}


void Environment::set(const string &name, const string &value) {
  iterator it = find(name);

  if (it == end()) add(new Variable(name, value));

  else {
    const SmartPointer<Entity> &e = it->second;
    if (e->getType() != Entity::VARIABLE)
      THROWS("'" << name << "' is not a variable in this context");

    e.cast<Variable>()->set(value);
  }
}


void Environment::setf(const char *key, const char *value, ...) {
  va_list ap;

  va_start(ap, value);
  set(key, String::vprintf(value, ap));
  va_end(ap);
}


bool Environment::eval(const Context &ctx) {
  if (!ctx.args.size()) THROW("Internal error: Missing arg 0");

  iterator it = find(ctx.args[0]);
  if (it == end()) {
    if (parent && parent != this) return parent->eval(ctx);
    return false;
  }

  localEval(ctx, *it->second);

  return true;
}


void Environment::localEval(const Context &ctx, Entity &e) {
  e.validate(ctx.args);

  if (e.evalArgs()) {
    Arguments args;
    args.push_back(ctx.args[0]);

    for (unsigned i = 1; i < ctx.args.size(); i++)
      args.push_back(ctx.handler.eval(ctx.args[i]));

    e.eval(Context(ctx, args));

  } else e.eval(ctx);
}


void Environment::evalHelp(const Context &ctx) {
  if (ctx.args.size() == 2) {
    iterator it = find(ctx.args[1]);
    if (it == end()) {
      if (parent && parent != this) parent->eval(ctx);
      return;
    }

    Entity &e = *it->second;
    if (e.getHelp() == "") ctx.stream << "No help for '" << ctx.args[0] << "'";
    else e.printHelpLine(ctx.stream);

  } else {
    bool printedName = false;

    for (iterator it = begin(); it != end(); it++) {
      if (it->second->getHelp() != "") {
        if (!printedName && name.size()) {
          ctx.stream << '\n' << name << ":\n";
          printedName = true;
        }
        it->second->printHelpLine(ctx.stream);
      }
    }

    if (parent && parent != this) parent->eval(ctx);
  }
}


void Environment::evalSet(const Context &ctx) {
  set(ctx.args[1], ctx.args[2]);
}


void Environment::evalUnset(const Context &ctx) {
  unset(ctx.args[1]);
}
