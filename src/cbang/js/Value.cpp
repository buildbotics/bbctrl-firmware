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

#include "Value.h"

#include <cbang/json/Writer.h>

using namespace cb::js;
using namespace cb;
using namespace std;


void Value::copyProperties(const Value &value) {
  SmartPointer<Value> props = value.getOwnPropertyNames();
  unsigned length = props->length();

  for (unsigned i = 0; i < length; i++) {
    string key = props->getString(i);
    set(key, value.get(key));
  }
}


void Value::write(JSON::Sink &sink) const {
  if (isObject()) {
    sink.beginDict();

    SmartPointer<Value> props = getOwnPropertyNames();
    for (unsigned i = 0; i < props->length(); i++) {
      string key = props->get(i)->toString();
      if (get(key)->isUndefined()) continue;
      sink.beginInsert(key);
      get(key)->write(sink);
    }

    sink.endDict();

  } else if (isArray()) {
    sink.beginList();

    for (unsigned i = 0; i < length(); i++) {
      if (get(i)->isUndefined()) continue;
      sink.beginAppend();
      get(i)->write(sink);
    }

    sink.endList();

  } else if (isNull()) sink.writeNull();
  else if (isNumber()) sink.write(toNumber());
  else if (isBoolean()) sink.writeBoolean(toBoolean());
  else if (isString()) sink.write(toString());
  else if (isFunction()) sink.write("[function]");
  else if (isUndefined()) sink.write("[undefined]");
}


void Value::write(ostream &stream) const {
  JSON::Writer writer(stream, 2);
  write(writer);
}
