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

#include "Arguments.h"

#include "Signature.h"
#include "Context.h"

#include <cbang/log/Logger.h>

using namespace cb::js;
using namespace std;


Arguments::Arguments(const v8::Arguments &args, const Signature &sig) :
  args(args), sig(sig), positional(args.Length()) {

  if (sig.size() && positional && args[positional - 1]->IsObject()) {
    keyWord = args[positional - 1];
    positional--;

    if (sig.isVariable()) return;

    // Validate key word arguments
    Value props = keyWord.getOwnPropertyNames();

    for (int i = 0; i < props.length(); i++) {
      string name = props.get(i).toString();
      if (!sig.has(name))
        THROWS("Invalid key word argument '" << name << "' when calling "
               << sig);
    }
  }
}


unsigned Arguments::getCount() const {
  return positional + (keyWord.isUndefined() ? 0 : keyWord.length());
}


string Arguments::getName(unsigned index) const {
  if (index < positional) return sig.keyAt(index);

  if (!keyWord.isUndefined()) {
    Value props = (v8::Handle<v8::Value>)
      keyWord.getV8Value()->ToObject()->GetOwnPropertyNames();

    if (index - positional < (unsigned)props.length())
      return props.get(index - positional).toString();
  }

  THROWS("Index " << index << " out of range");
}


bool Arguments::has(unsigned index) const {
  return index < getCount();
}


bool Arguments::has(const string &name) const {
  unsigned index = sig.indexOf(name);
  return index < positional || (!keyWord.isUndefined() && keyWord.has(name));
}


Value Arguments::get(unsigned index) const {
  if (!sig.isVariable() && sig.size() <= index)
    THROWS("Index " << index << " out of range");
  if (index < positional) return args[index];
  return keyWord.get(index - positional);
}


Value Arguments::get(const string &name) const {
  unsigned index = sig.indexOf(name);
  Value defaultValue = sig.get(index);

  if (index < positional) return args[index];
  if (!keyWord.isUndefined()) return keyWord.get(name, defaultValue);

  return defaultValue;
}


bool Arguments::getBoolean(const string &name) const {
  return get(name).toBoolean();
}


double Arguments::getNumber(const string &name) const {
  return get(name).toNumber();
}


int32_t Arguments::getInt32(const string &name) const {
  return get(name).toInt32();
}


uint32_t Arguments::getUint32(const string &name) const {
  return get(name).toUint32();
}


string Arguments::getString(const string &name) const {
  return get(name).toString();
}


bool Arguments::getBoolean(unsigned index) const {
  return get(index).toBoolean();
}


double Arguments::getNumber(unsigned index) const {
  return get(index).toNumber();
}


int32_t Arguments::getInt32(unsigned index) const {
  return get(index).toInt32();
}


uint32_t Arguments::getUint32(unsigned index) const {
  return get(index).toUint32();
}


string Arguments::getString(unsigned index) const {
  return get(index).toString();
}


string Arguments::toString() const {
  ostringstream str;
  str << "(" << *this << ")";
  return str.str();
}


ostream &Arguments::write(ostream &stream, const string &separator) const {
  Value JSON = Context::current().getGlobal().get("JSON");
  Value stringify = JSON.get("stringify");

  for (unsigned i = 0; i < getCount(); i++) {
    Value value = get(i);

    if (i) stream << separator;

    if (value.isObject()) stream << stringify.call(JSON, value);
    else stream << value;
  }

  return stream;
}
