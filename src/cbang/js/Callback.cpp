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

#include "Callback.h"
#include "Isolate.h"

#include <cbang/Exception.h>
#include <cbang/SStream.h>

using namespace std;
using namespace cb::js;


Callback::Callback(const Signature &sig) :
  sig(sig), data(v8::External::New(v8::Isolate::GetCurrent(), this)),
  function(v8::FunctionTemplate::New
           (v8::Isolate::GetCurrent(), &Callback::callback, data)) {
}


void Callback::raise(const v8::FunctionCallbackInfo<v8::Value> &info,
                     const std::string &_msg) {
  v8::Local<v8::Value> msg =
    v8::String::NewFromUtf8(info.GetIsolate(), _msg.c_str());
  info.GetReturnValue().Set(info.GetIsolate()->ThrowException(msg));
}


void Callback::callback(const v8::FunctionCallbackInfo<v8::Value> &info) {
  if (Isolate::shouldQuit()) {
    raise(info, "Interrupted");
    return;
  }

  Callback *cb =
    static_cast<Callback *>(v8::External::Cast(*info.Data())->Value());

  try {
    Value ret = (*cb)(Arguments(info, cb->sig));
    info.GetReturnValue().Set(ret.getV8Value());

  } catch (const Exception &e) {
    raise(info, SSTR(e));

  } catch (const std::exception &e) {
    raise(info, e.what());

  } catch (...) {
    raise(info, "Unknown exception");
  }
}
