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

#ifndef CBANG_REF_COUNTER_H
#define CBANG_REF_COUNTER_H

#include "Deallocators.h"

#include <cbang/os/Mutex.h>


namespace cb {
  /// This class is used by SmartPointer to count pointer references
  class RefCounterBase {
    long count;

  public:
    RefCounterBase(long count) : count(count) {}
    virtual ~RefCounterBase() {}

    virtual long getCount() const {return count;}
    virtual void incCount() {count++;}
    virtual void decCount() {count--;}
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class RefCounter : public RefCounterBase {
  protected:
    T *ptr;

  public:
    RefCounter(T *ptr) : RefCounterBase(1), ptr(ptr) {}

    // From RefCounterBase
    void decCount() {
      RefCounterBase::decCount();
      if (!RefCounterBase::getCount()) {
        release();
        delete this;
      }
    }

    void release() {
      Dealloc_T::dealloc(ptr);
      ptr = 0;
    }
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class ProtectedRefCounter : public RefCounter<T, Dealloc_T>, public Mutex {
  public:
    ProtectedRefCounter(T *ptr) : RefCounter<T, Dealloc_T>(ptr) {}

    // From RefCounterBase
    long getCount() const
    {lock(); long x = RefCounterBase::getCount(); unlock(); return x;}
    void incCount() {lock(); RefCounterBase::incCount(); unlock();}

    void decCount() {
      lock();

      RefCounterBase::decCount();
      if (!RefCounterBase::getCount()) {
        RefCounter<T, Dealloc_T>::release();
        unlock();
        delete this;

      } else unlock();
    }
  };
}
#endif
