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

#ifndef CBANG_THREAD_POOL_FUNC_H
#define CBANG_THREAD_POOL_FUNC_H

#include "ThreadPool.h"


namespace cb {
  template<class T, typename METHOD_T = void (T::*)()>
  class ThreadPoolFunc : public ThreadPool {
    T *obj;          // pointer to object
    METHOD_T method; // pointer to method

  public:
    /**
     * Construct a ThreadPool which will execute a method of class T.
     *
     * @param size The number of threads in the pool.
     * @param obj The class instance.
     * @param method A pointer to the method.
     */
    ThreadPoolFunc(unsigned size, T *obj, METHOD_T method) :
      ThreadPool(size), obj(obj), method(method) {}

  private:
    /// Passes the polymorphic call to run on to the target class.
    virtual void run() {(*obj.*method)();}
  };
}

#endif // CBANG_THREAD_POOL_FUNC_H
