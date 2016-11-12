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

#ifndef CB_DUK_CONTEXT_H
#define CB_DUK_CONTEXT_H

#include "Callback.h"
#include "Signature.h"
#include "MethodCallback.h"

#include <cbang/SmartPointer.h>
#include <cbang/io/InputSource.h>

#include <string>
#include <vector>

typedef struct duk_hthread duk_context;


namespace cb {
  namespace duk {
    class Object;
    class Array;
    class Module;

    class Context {
    protected:
      duk_context *ctx;
      const bool deallocate;

      std::vector<SmartPointer<Callback> > callbacks;

    public:
      Context(duk_context *ctx) : ctx(ctx), deallocate(false) {}
      Context();
      ~Context();

      duk_context *getContext() const {return ctx;}

      int top() const;
      int topIndex() const;

      void pop(unsigned n = 1) const;
      void dup(int index = -1) const;

      int getType(int index = -1) const;
      bool isArray(int index = -1) const;
      bool isObject(int index = -1) const;
      bool isBoolean(int index = -1) const;
      bool isError(int index = -1) const;
      bool isNull(int index = -1) const;
      bool isNumber(int index = -1) const;
      bool isPointer(int index = -1) const;
      bool isString(int index = -1) const;
      bool isUndefined(int index = -1) const;

      Array toArray(int index = -1);
      Object toObject(int index = -1);
      bool toBoolean(int index = -1);
      int toInteger(int index = -1);
      double toNumber(int index = -1);
      void *toPointer(int index = -1);
      std::string toString(int index = -1);

      Object pushGlobalObject();
      Object pushCurrentFunction();
      Array pushArray();
      Object pushObject();
      void pushUndefined();
      void pushNull();
      void pushBoolean(bool x);
      void pushPointer(void *x);
      void push(int x);
      void push(unsigned x);
      void push(double x);
      void push(const char *x);
      void push(const std::string &x);
      void push(const SmartPointer<Callback> &cb);
      void push(const Variant &value);

      template <class T>
      void push(const Signature &sig, T *obj,
                typename MethodCallback<T>::member_t member) {
        push(new MethodCallback<T>(sig, obj, member));
      }

      void defineGlobal(Module &module);
      void define(Module &module);

      void eval(const InputSource &source);
      void raise(const std::string &msg) const;
      void error(const std::string &msg, int code = 0) const;
    };
  }
}

#endif // CB_DUK_CONTEXT_H
