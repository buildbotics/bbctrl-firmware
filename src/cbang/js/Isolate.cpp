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

#include "Isolate.h"

#include "V8.h"

using namespace cb::js;


namespace cb {
  namespace js {
    class Isolate::Scope::Private {
      v8::Locker locker;
      v8::Isolate::Scope isolateScope;
      v8::HandleScope handleScope;

    public:
      Private(v8::Isolate *isolate) : locker(isolate), isolateScope(isolate) {}
    };
  }
}


Isolate::Scope::Scope(Isolate &iso) : pri(new Private(iso.getIsolate())) {}
Isolate::Scope::~Scope() {delete pri;}


Isolate::Isolate() : isolate(v8::Isolate::New()), interrupted(false) {
  isolate->SetData(this);
}


Isolate::~Isolate() {
  isolate->Dispose();
}


void Isolate::interrupt() {
  interrupted = true;
  v8::V8::TerminateExecution(isolate);
}


Isolate *Isolate::current() {
  return (Isolate *)v8::Isolate::GetCurrent()->GetData();
}


bool Isolate::shouldQuit() {
  Isolate *iso = current();
  return iso && iso->wasInterrupted();
}
