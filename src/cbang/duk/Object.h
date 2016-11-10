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

#ifndef CB_DUK_OBJECT_H
#define CB_DUK_OBJECT_H

#include "Context.h"

#include <string>


namespace cb {
  namespace duk {
    class Object {
      Context &ctx;
      int index;

    public:
      Object(Context &ctx, int index) : ctx(ctx), index(index) {}

      bool has(const std::string &key) const;

      bool toBoolean(const std::string &key) const;
      int toInteger(const std::string &key) const;
      double toNumber(const std::string &key) const;
      void *toPointer(const std::string &key) const;
      std::string toString(const std::string &key) const;

      void setNull(const std::string &key);
      void setBoolean(const std::string &key, bool x);
      void set(const std::string &key, int x);
      void set(const std::string &key, double x);
      void set(const std::string &key, const std::string &x);
      void set(const std::string &key, Object &obj);

      template <class T>
      void set(const Signature &sig, T *obj,
               typename MethodCallback<T>::member_t member) {
        ctx.push(sig.getName());
        ctx.push(sig, obj, member);
        ctx.putProp(index);
      }
    };
  }
}

#endif // CB_DUK_OBJECT_H
