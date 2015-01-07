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

#ifndef CB_JSON_BUILDER_H
#define CB_JSON_BUILDER_H

#include "Sync.h"
#include "Value.h"

#include <vector>


namespace cb {
  namespace JSON {
    class Builder : public Sync {
      std::vector<ValuePtr> stack;
      bool appendNext;
      std::string nextKey;

    public:
      Builder(const ValuePtr &root = 0);

      ValuePtr getRoot() const;
      void clear() {stack.clear();}

      // From Sync
      void writeNull();
      void writeBoolean(bool value);
      void write(double value);
      void write(const std::string &value);
      void beginList(bool simple = false);
      void beginAppend();
      void endList();
      void beginDict(bool simple = false);
      bool has(const std::string &key) const;
      void beginInsert(const std::string &key);
      void endDict();

    protected:
      void add(const ValuePtr &value);
      void assertNotPending();
      bool shouldAppend();
      bool shouldInsert() {return !nextKey.empty();}
    };
  }
}

#endif // CB_JSON_BUILDER_H

