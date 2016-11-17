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

#ifndef CB_JS_SINK_H
#define CB_JS_SINK_H

#include "Function.h"
#include "MethodCallback.h"

#include <cbang/SmartPointer.h>
#include <cbang/json/NullSink.h>


namespace cb {
  namespace js {
    class Sink : public JSON::NullSink {
    public:
      // From JSON::Sink
      using JSON::NullSink::insert;
      using JSON::NullSink::write;

      virtual void write(const Function &func) = 0;

      template <class T>
      void insert(const Signature &sig, T *obj,
                  typename MethodCallback<T>::member_t member) {
        beginInsert(sig.getName());
        write(Function(new MethodCallback<T>(sig, obj, member)));
      }
    };
  }
}

#endif // CB_JS_SINK_H
