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

#include <cbang/log/Logger.h>
#include <cbang/util/DefaultCatch.h>

using namespace cb::chakra;
using namespace cb;
using namespace std;


JSImpl::JSImpl() {
  JsCreateRuntime(JsRuntimeAttributeNone, 0, &runtime);
  JsCreateContext(runtime, &context);
  JsSetCurrentContext(context);
}


JSImpl::~JSImpl() {
  try {
    JsSetCurrentContext(JS_INVALID_REFERENCE);
    JsDisposeRuntime(runtime);
  } CATCH_ERROR;
}


void JSImpl::define(js::Module &mod) {
}


void JSImpl::import(const string &module, const string &as) {
}


void JSImpl::exec(const InputSource &source) {
  JsEnableRuntimeExecution(runtime);

  // Execute script
  Value name(source.getName());

  string s = source.toString();
  JsValueRef script;
  CHAKRA_CHECK(JsCreateExternalArrayBuffer
               ((void *)CPP_TO_C_STR(s), s.length(), 0, 0, &script));

  JsRun(script, 0, name, JsParseScriptAttributeNone, 0);

  // Check for errors
  if (Value::hasException()) {
    Value ex = Value::getException();
    ostringstream msg;

    if (ex.has("stack")) msg << ex.get("stack").toString();
    else if (ex.has("line"))
      msg << ex.toString() << "\n  at " << source.getName() << ':'
          << ex.get("line").toInteger() << ':'
          << ex.get("column").toInteger();

    THROW(msg.str());
  }
}


void JSImpl::interrupt() {JsDisableRuntimeExecution(runtime);}
