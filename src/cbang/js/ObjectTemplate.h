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

#ifndef CB_JS_OBJECT_TEMPLATE_H
#define CB_JS_OBJECT_TEMPLATE_H

#include "Value.h"
#include "Callback.h"
#include "FunctionCallback.h"
#include "MethodCallback.h"
#include "VoidMethodCallback.h"

#include <cbang/SmartPointer.h>

#include "V8.h"

#include <string>
#include <vector>


namespace cb {
  namespace js {
    class ObjectTemplate {
      v8::Handle<v8::ObjectTemplate> tmpl;
      std::vector<SmartPointer<Callback> > callbacks;

    public:
      ObjectTemplate();

      Value create() const;

      void set(const std::string &name, const Value &value);
      void set(const std::string &name, const ObjectTemplate &tmpl);
      void set(const std::string &name, const Callback &callback);
      void set(const Signature &sig, FunctionCallback::func_t func);

      template <class T>
      void set(const Signature &sig, T *object,
               typename MethodCallback<T>::member_t member) {
        SmartPointer<Callback> cb = new MethodCallback<T>(sig, object, member);
        callbacks.push_back(cb);
        set(sig.getName(), *cb);
      }

      template <class T>
      void set(const Signature &sig, T *object,
               typename VoidMethodCallback<T>::member_t member) {
        SmartPointer<Callback> cb =
          new VoidMethodCallback<T>(sig, object, member);
        callbacks.push_back(cb);
        set(sig.getName(), *cb);
      }

      v8::Handle<v8::ObjectTemplate> getTemplate() const {return tmpl;}
    };
  }
}

#endif // CB_JS_OBJECT_TEMPLATE_H

