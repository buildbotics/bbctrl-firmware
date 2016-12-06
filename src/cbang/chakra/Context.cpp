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
#include "JSImpl.h"

#include <string.h>

using namespace cb::chakra;
using namespace cb;
using namespace std;


Context::Context(JSImpl &impl) : impl(impl) {
  CHAKRA_CHECK(JsCreateContext(impl.getRuntime(), &context));
  CHAKRA_CHECK(JsSetContextData(context, this));
  enter();
}


Context &Context::current() {
  JsContextRef ref;
  CHAKRA_CHECK(JsGetCurrentContext(&ref));

  Context *ctx;
  CHAKRA_CHECK(JsGetContextData(ref, (void **)&ctx));

  return *ctx;
}


void Context::enter() {CHAKRA_CHECK(JsSetCurrentContext(context));}
void Context::leave() {JsSetCurrentContext(JS_INVALID_REFERENCE);}


Value Context::eval(const string &path, const string &code) {
  impl.enable();
  enter();

  JsValueRef result;
  JsRun(Value::createArrayBuffer(code), 0, Value(path),
        JsParseScriptAttributeNone, &result);

  // Check for errors
  if (Value::hasException()) {
    Value ex = Value::getException();
    ostringstream msg;

    if (ex.isObject() && ex.has("stack")) msg << ex.getString("stack");
    else if (ex.isObject() && ex.has("line"))
      msg << ex.toString() << "\n  at " << path << ':'
          << ex.getInteger("line") << ':'
          << ex.getInteger("column");
    else msg << ex.toString();

    THROW(msg.str());
  }

  return result;
}


Value Context::eval(const InputSource &source) {
  return eval(source.getName(), source.toString());
}
