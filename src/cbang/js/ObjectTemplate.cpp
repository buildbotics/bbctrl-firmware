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

#include "ObjectTemplate.h"

using namespace std;
using namespace cb::js;


ObjectTemplate::ObjectTemplate() :
  tmpl(v8::ObjectTemplate::New(v8::Isolate::GetCurrent())) {}


Value ObjectTemplate::create() const {
  return v8::Handle<v8::Value>(tmpl->NewInstance());
}


void ObjectTemplate::set(const string &name, const Value &value) {
  tmpl->Set(v8::String::NewFromUtf8(v8::Isolate::GetCurrent(), name.c_str(),
                                    v8::String::kInternalizedString,
                                    name.length()), value.getV8Value());
}


void ObjectTemplate::set(const string &name, const ObjectTemplate &tmpl) {
  this->tmpl->Set(v8::String::NewFromUtf8
                  (v8::Isolate::GetCurrent(), name.c_str(),
                   v8::String::kInternalizedString, name.length()),
                  tmpl.getTemplate());
}


void ObjectTemplate::set(const string &name, const Callback &callback) {
  tmpl->Set(v8::String::NewFromUtf8
            (v8::Isolate::GetCurrent(), name.c_str(),
             v8::String::kInternalizedString, name.length()),
            callback.getTemplate());
}


void ObjectTemplate::set(const Signature &sig, FunctionCallback::func_t func) {
  SmartPointer<Callback> cb = new FunctionCallback(sig, func);
  callbacks.push_back(cb);
  set(sig.getName(), *cb);
}
