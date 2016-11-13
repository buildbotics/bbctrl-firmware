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
#include "RawMethodCallback.h"

#include <cbang/SmartPointer.h>
#include <cbang/io/InputSource.h>
#include <cbang/json/Sink.h>

#include <string>
#include <vector>

typedef struct duk_hthread duk_context;


namespace cb {
  namespace JSON {class Value;}

  namespace duk {
    class Object;
    class Array;
    class Enum;
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

      duk_context *getContext() {return ctx;}

      int type(int index = -1);

      bool has(int index);
      bool has(int index, int i);
      bool has(int index, const std::string &key);

      bool get(int index);
      bool get(int index, int i);
      bool get(int index, const std::string &key);
      bool getGlobal(const std::string &key);

      bool put(int index);
      bool put(int index, int i);
      bool put(int index, const std::string &key);
      bool putGlobal(const std::string &key);

      unsigned top();
      unsigned topIndex();
      unsigned normalize(int index);

      void pop(unsigned n = 1);
      void dup(int index = -1);

      void compile(const std::string &filename);
      void compile(const std::string &filename, const std::string &code);
      void call(int nargs);
      void callMethod(int nargs);

      unsigned length(int index = -1);
      Enum enumerate(int index = -1, int flags = 0);
      bool next(int index = -1, bool getValue = true);
      void write(int index, JSON::Sink &sink);

      bool isJSON(int index = -1);
      bool isArray(int index = -1);
      bool isObject(int index = -1);
      bool isBoolean(int index = -1);
      bool isError(int index = -1);
      bool isNull(int index = -1);
      bool isNumber(int index = -1);
      bool isPointer(int index = -1);
      bool isString(int index = -1);
      bool isUndefined(int index = -1);

      SmartPointer<JSON::Value> toJSON(int index = -1);
      Array toArray(int index = -1);
      Object toObject(int index = -1);
      bool toBoolean(int index = -1);
      int toInteger(int index = -1);
      double toNumber(int index = -1);
      void *toPointer(int index = -1);
      std::string toString(int index = -1);

      Array getArray(int index = -1);
      Object getObject(int index = -1);
      bool getBoolean(int index = -1);
      int getInteger(int index = -1);
      double getNumber(int index = -1);
      void *getPointer(int index = -1);
      std::string getString(int index = -1);

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

      template <class T>
      void push(const Signature &sig, T *obj,
                typename MethodCallback<T>::member_t member) {
        push(new MethodCallback<T>(sig, obj, member));
      }

      template <class T>
      void push(T *obj, typename RawMethodCallback<T>::member_t member) {
        push(new RawMethodCallback<T>(obj, member));
      }

      void defineGlobal(Module &module);
      void define(Module &module);

      void eval(const InputSource &source);
      void raise(const std::string &msg);
      void error(const std::string &msg, int code = 0);
      std::string dump();
    };
  }
}

#endif // CB_DUK_CONTEXT_H
