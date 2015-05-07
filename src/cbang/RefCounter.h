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
  class RefCounter {
  public:
    virtual ~RefCounter() {}

    virtual long getCount() const = 0;
    virtual void incCount() = 0;
    virtual void decCount(const void *ptr) = 0;
  };


  class RefCounterPhonyImpl : public RefCounter {
    static RefCounterPhonyImpl singleton;

  public:
    static RefCounter *create() {return &singleton;}

    // From RefCounter
    long getCount() const {return 1;}
    void incCount() {};
    void decCount(const void *ptr) {}
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class RefCounterImpl : public RefCounter {
  protected:
    long count;

  public:
    RefCounterImpl() : count(1) {}
    static RefCounter *create() {return new RefCounterImpl;}

    // From RefCounter
    long getCount() const {return count;}
    void incCount() {count++;}

    void decCount(const void *ptr) {
      if (!--count) {
        Dealloc_T::dealloc((T *)ptr);
        delete this;
      }
    }
  };


  template<typename T, class Dealloc_T = DeallocNew<T> >
  class ProtectedRefCounterImpl :
    public RefCounterImpl<T, Dealloc_T>, public Mutex {
  protected:
    using RefCounterImpl<T>::count;

  public:
    static RefCounter *create() {return new ProtectedRefCounterImpl;}

    // From RefCounterImpl
    long getCount() const {
      lock();
      long x = count;
      unlock();
      return x;
    }

    void incCount() {lock(); count++; unlock();}

    void decCount(const void *ptr) {
      lock();

      if (!--count) {
        Dealloc_T::dealloc((T *)ptr);
        unlock();
        delete this;

      } else unlock();
    }
  };
}
#endif
