/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

              Copyright (c) 2003-2014, Cauldron Development LLC
                 Copyright (c) 2003-2014, Stanford University
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

#include "Dict.h"

#include "Null.h"
#include "Boolean.h"
#include "Number.h"
#include "String.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cctype>

using namespace std;
using namespace cb::JSON;


ValuePtr Dict::copy(bool deep) const {
  ValuePtr c = new Dict;

  for (unsigned i = 0; i < size(); i++)
    c->insert(keyAt(i), deep ? get(i)->copy(true) : get(i));

  return c;
}


void Dict::insertNull(const string &key) {
  insert(key, Null::instancePtr());
}


void Dict::insertBoolean(const string &key, bool value) {
  insert(key, new Boolean(value));
}


void Dict::insert(const string &key, double value) {
  insert(key, new Number(value));
}


void Dict::insert(const string &key, const string &value) {
  insert(key, new String(value));
}


void Dict::insert(const string &key, const ValuePtr &value) {
  if (value->isList() || value->isDict()) simple = false;
  OrderedDict<ValuePtr>::insert(key, value);
}


void Dict::write(Sync &sync) const {
  sync.beginDict(isSimple());

  for (const_iterator it = begin(); it != end(); it++) {
    sync.beginInsert(it->first);
    it->second->write(sync);
  }

  sync.endDict();
}
