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

#ifndef CB_JS_OBJECT_H
#define CB_JS_OBJECT_H

#include "Value.h"
#include "FunctionCallback.h"
#include "MethodCallback.h"
#include "VoidMethodCallback.h"


namespace cb {
  namespace js {
    // Forward Declarations
    template <typename T> class ObjectBase;

    // Type Definitions
    typedef ObjectBase<v8::Handle<v8::Value> > Object;
    typedef ObjectBase<v8::Persistent<v8::Value> > PersistentObject;


    template <typename T = v8::Handle<v8::Value> >
    class ObjectBase : public ValueBase<T> {
      std::vector<SmartPointer<Callback> > callbacks;

    public:
      using ValueBase<T>::assertObject;

      ObjectBase() : ValueBase<T>(v8::Object::New()) {}
      ObjectBase(const Value &value) : ValueBase<T>(value) {assertObject();}
      ObjectBase(const PersistentValue &value) :
      ValueBase<T>(value) {assertObject();}
      ObjectBase(const T &value) : ValueBase<T>(value) {assertObject();}

      using ValueBase<T>::set;


      void set(const std::string &name, const Callback &callback) {
        set(name, ValueBase<T>(callback.getTemplate()->GetFunction()));
      }


      void set(const Signature &sig, FunctionCallback::func_t func) {
        SmartPointer<Callback> cb = new FunctionCallback(sig, func);
        callbacks.push_back(cb);
        set(sig.getName(), *cb);
      }


      template <class C>
      void set(const Signature &sig, C *object,
               typename MethodCallback<C>::member_t member) {
        SmartPointer<Callback> cb = new MethodCallback<C>(sig, object, member);
        callbacks.push_back(cb);
        set(sig.getName(), *cb);
      }


      template <class C>
      void set(const Signature &sig, C *object,
               typename VoidMethodCallback<C>::member_t member) {
        SmartPointer<Callback> cb =
          new VoidMethodCallback<C>(sig, object, member);
        callbacks.push_back(cb);
        set(sig.getName(), *cb);
      }
    };
  }
}

#endif // CB_JS_OBJECT_H
