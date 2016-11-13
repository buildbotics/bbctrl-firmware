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

#ifndef CB_DUK_ARRAY_H
#define CB_DUK_ARRAY_H

#include "Context.h"

#include <string>


namespace cb {
  namespace duk {
    class Object;

    class Array {
      Context &ctx;
      unsigned index;

    public:
      Array(Context &ctx, int index);

      unsigned getIndex() const {return index;}

      bool has(int i);
      bool get(int i);
      bool put(int i);

      unsigned length();

      void write(JSON::Sink &sink);
      void appendValues(JSON::Sink &sink);

      int type(int i);
      bool isArray(int i);
      bool isObject(int i);
      bool isBoolean(int i);
      bool isError(int i);
      bool isNull(int i);
      bool isNumber(int i);
      bool isPointer(int i);
      bool isString(int i);
      bool isUndefined(int i);

      Array toArray(int i);
      Object toObject(int i);
      bool toBoolean(int i);
      int toInteger(int i);
      double toNumber(int i);
      void *toPointer(int i);
      std::string toString(int i);

      void setArray(int i);
      void setObject(int i);
      void setUndefined(int i);
      void setNull(int i);
      void setBoolean(int i, bool x);
      void set(int i, int x);
      void set(int i, unsigned x);
      void set(int i, double x);
      void set(int i, const std::string &x);
      void set(int i, Object &obj);
      void set(int i, Array &ary);
    };
  }
}

#endif // CB_DUK_ARRAY_H
