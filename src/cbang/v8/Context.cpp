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

using namespace cb;
using namespace cb::gv8;
using namespace std;


Context::Context() : context(v8::Context::New(0, v8::ObjectTemplate::New())) {}


Value Context::eval(const InputSource &src) {
  v8::HandleScope handleScope;
  Context::Scope ctxScope(*this);

  // Get script origin
  v8::Local<v8::String> origin;
  string filename = src.getName();
  if (!filename.empty())
    origin = v8::String::New(filename.c_str(), filename.length());

  // Get script source
  string s = src.toString();
  v8::Local<v8::String> source = v8::String::New(s.c_str(), s.length());

  // Compile
  v8::TryCatch tryCatch;
  v8::Handle<v8::Script> script = v8::Script::Compile(source, origin);
  if (tryCatch.HasCaught()) translateException(tryCatch, false);

  // Execute
  v8::Handle<v8::Value> ret = script->Run();
  if (tryCatch.HasCaught()) translateException(tryCatch, true);

  // Return result
  return handleScope.Close(ret);
}


void Context::translateException(const v8::TryCatch &tryCatch,
                                 bool useStack) {
  v8::HandleScope handleScope;

  if (useStack && !tryCatch.StackTrace().IsEmpty())
    throw Exception(Value(tryCatch.StackTrace()).toString());

  if (tryCatch.Exception()->IsNull()) throw Exception("Interrupted");

  string msg = Value(tryCatch.Exception()).toString();

  v8::Handle<v8::Message> message = tryCatch.Message();
  if (message.IsEmpty()) throw Exception(msg);

  string filename = Value(message->GetScriptResourceName()).toString();
  int line = message->GetLineNumber();
  int col = message->GetStartColumn();

  throw Exception(msg, FileLocation(filename, line, col));
}
