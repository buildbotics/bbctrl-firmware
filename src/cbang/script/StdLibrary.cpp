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

#include "StdLibrary.h"

#include "Functor.h"

#include <cbang/String.h>
#include <cbang/time/Time.h>
#include <cbang/time/Timer.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::Script;

namespace cb {SINGLETON_DECL(StdLibrary);}


// Have to pass 'this' as parent pointer to Environment to stop infinite loop.
StdLibrary::StdLibrary(Inaccessible) : Environment("Standard Commands", this) {
#define FUNC(name, func, min, max, help, argHelp, evalArgs)             \
  add(new Functor(name, &StdLibrary::func, min, max, help, argHelp, evalArgs))

  add(new Variable("prompt", "> "));

  FUNC("if", evalIf, 2, 3, "If 'cond' evaluates to a non-empty "
       "string then evalute 'expr1' otherwise, if provided, evaluate 'expr2'",
       "<cond> <expr1> [expr2]", false);
  FUNC("not", evalNot, 1, 1, "Invert the truth value of the argument",
       "<expr>", true);
  FUNC("eval", evalEval, 0, ~0, "Evaluate all arguments", "[expr]...", true);
  FUNC("clear", evalClear, 0, 0, "Clear the screen", "", true);
  FUNC("date", evalDate, 0, 1, "Print the date and time.  Optionally, with "
       "'format'.  See: man strftime", "[format]", true);
  FUNC("sleep", evalSleep, 1, 1, "Sleep for a number of seconds", "<seconds>",
       true);
  FUNC("add", evalAdd, 2, 2, "Add two values", "<number> <number>", true);
  FUNC("sub", evalSub, 2, 2, "Subtract two values", "<number> <number>", true);
  FUNC("mul", evalMul, 2, 2, "Multiply two values", "<number> <number>", true);
  FUNC("div", evalDiv, 2, 2, "Divide two values", "<number> <number>", true);
  FUNC("eq", evalEq, 2, 2, "True if arguments are equal", "<string> <string>",
       true);
  FUNC("neq", evalNeq, 2, 2, "True if arguments are not equal",
       "<string> <string>", true);
  FUNC("less", evalLess, 2, 2, "True the first argument is lexigraphically "
       "less than the second", "<string> <string>", true);
}


void StdLibrary::evalIf(const Context &ctx) {
  string cond = ctx.handler.eval(ctx.args[1]);
  if (cond != "" && cond != "false") Handler::eval(ctx, ctx.args[2]);
  else if (ctx.args.size() == 4) Handler::eval(ctx, ctx.args[3]);
}


void StdLibrary::evalNot(const Context &ctx) {
  string expr = ctx.args[1];
  if (expr != "" && expr != "false") ctx.stream << "false";
  else ctx.stream << "true";
}


void StdLibrary::evalEval(const Context &ctx) {
  for (unsigned i = 1; i < ctx.args.size(); i++)
    Handler::eval(ctx, ctx.args[i]);
}


void StdLibrary::evalClear(const Context &ctx) {
  ctx.stream << "\033[H\033[2J";
}


void StdLibrary::evalDate(const Context &ctx) {
  if (1 < ctx.args.size()) ctx.stream << Time(ctx.args[1]);
  else ctx.stream << Time();
}


void StdLibrary::evalSleep(const Context &ctx) {
  Timer::sleep(String::parseDouble(ctx.args[1]));
}


void StdLibrary::evalAdd(const Context &ctx) {
  double x = String::parseDouble(ctx.args[1]);
  double y = String::parseDouble(ctx.args[2]);
  ctx.stream << (x + y);
}


void StdLibrary::evalSub(const Context &ctx) {
  double x = String::parseDouble(ctx.args[1]);
  double y = String::parseDouble(ctx.args[2]);
  ctx.stream << (x - y);
}


void StdLibrary::evalMul(const Context &ctx) {
  double x = String::parseDouble(ctx.args[1]);
  double y = String::parseDouble(ctx.args[2]);
  ctx.stream << (x * y);
}


void StdLibrary::evalDiv(const Context &ctx) {
  double x = String::parseDouble(ctx.args[1]);
  double y = String::parseDouble(ctx.args[2]);
  ctx.stream << (x / y);
}


void StdLibrary::evalEq(const Context &ctx) {
  ctx.stream << (ctx.args[1] == ctx.args[2] ? "true" : "false");
}


void StdLibrary::evalNeq(const Context &ctx) {
  ctx.stream << (ctx.args[1] != ctx.args[2] ? "true" : "false");
}


void StdLibrary::evalLess(const Context &ctx) {
  ctx.stream << (ctx.args[1] < ctx.args[2] ? "true" : "false");
}
