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

#ifndef CBANG_JSON_DICT_H
#define CBANG_JSON_DICT_H

#include "Value.h"

#include <cbang/util/OrderedDict.h>


namespace cb {
  namespace JSON {
    class Dict : public Value, public OrderedDict<ValuePtr> {
      bool simple;

    public:
      Dict() : simple(true) {}

      // From OrderedDict<ValuePtr>
      using OrderedDict<ValuePtr>::get;
      using OrderedDict<ValuePtr>::has;
      using OrderedDict<ValuePtr>::operator[];

      // From Value
      ValueType getType() const {return JSON_DICT;}
      ValuePtr copy(bool deep = false) const;
      bool isSimple() const {return simple;}
      using Value::getDict;
      Dict &getDict() {return *this;}
      const Dict &getDict() const {return *this;}
      unsigned size() const {return OrderedDict<ValuePtr>::size();}
      const std::string &keyAt(unsigned i) const
      {return OrderedDict<ValuePtr>::keyAt(i);}
      int indexOf(const std::string &key) const {return lookup(key);}
      const ValuePtr &get(unsigned i) const
      {return OrderedDict<ValuePtr>::get(i);}
      const ValuePtr &get(const std::string &key) const
      {return OrderedDict<ValuePtr>::get(key);}
      void insertNull(const std::string &key);
      void insertBoolean(const std::string &key, bool value);
      void insert(const std::string &key, double value);
      void insert(const std::string &key, const std::string &value);
      void insert(const std::string &key, const ValuePtr &value);
      void write(Sink &sink) const;
    };
  }
}

#endif // CBANG_JSON_DICT_H

