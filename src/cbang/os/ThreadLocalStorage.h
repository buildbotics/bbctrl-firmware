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

#ifndef CBANG_THREAD_LOCAL_STORAGE_H
#define CBANG_THREAD_LOCAL_STORAGE_H

#include "Mutex.h"
#include "Thread.h"

#include <cbang/StdTypes.h>

#include <cbang/util/SmartLock.h>

#include <map>

namespace cb {
  template <typename T>
  class ThreadLocalStorage : public Mutex {
  protected:
    typedef std::map<uint64_t, T> storage_t;
    storage_t storage;

  public:
    T &get() {
      SmartLock lock(this);

      typename storage_t::iterator it = storage.find(Thread::self());
      if (it == storage.end())
        it = storage.insert
          (typename storage_t::value_type(Thread::self(), T())).first;

      return it->second;
    }

    T &get(T defaultValue) {
      SmartLock lock(this);

      typename storage_t::iterator it = storage.find(Thread::self());
      if (it == storage.end())
        it = storage.insert
          (typename storage_t::value_type(Thread::self(), defaultValue)).first;

      return it->second;
    }

    bool isSet() const {
      SmartLock lock(this);
      return storage.find(Thread::self()) != storage.end();
    }

    void set(const T &value) {
      SmartLock lock(this);

      typename storage_t::iterator it = storage.find(Thread::self());
      if (it == storage.end())
        storage.insert(typename storage_t::value_type(Thread::self(), value));
      else it->second = value;
    }

    void clear() {
      SmartLock lock(this);
      typename storage_t::iterator it = storage.find(Thread::self());
      if (it != storage.end()) storage.erase(it);
    }
  };
}

#endif // CBANG_THREAD_LOCAL_STORAGE_H

