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

#include "StdModule.h"
#include "Javascript.h"

#include <iostream>

using namespace cb::js;
using namespace cb;
using namespace std;


void StdModule::define(Sink &exports) {
  exports.insert("require(id)", this, &StdModule::require);
  exports.insert("print(...)", this, &StdModule::print);
}


SmartPointer<Value> StdModule::require(Callback &cb, Value &args) {
  return js.require(cb, args);
}


void StdModule::print(const Value &args, Sink &sink) {
  for (unsigned i = 0; i < args.length(); i++) {
    if (i) cout << ' ';
    cout << args.get(i)->toString();
  }
}
