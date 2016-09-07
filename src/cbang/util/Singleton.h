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

#ifndef CBANG_SINGLETON_H
#define CBANG_SINGLETON_H

#include "Base.h"

#include <cbang/Exception.h>

#include <typeinfo>
#include <vector>


namespace cb {
  class Inaccessible {
    Inaccessible() {}
    template <typename T> friend class Singleton;
  };


  class SingletonDealloc {
    static SingletonDealloc *singleton;
    typedef std::vector<Base *> singletons_t;
    singletons_t singletons;

    SingletonDealloc() {}
  public:

    static SingletonDealloc &instance();

    void add(Base *singleton) {singletons.push_back(singleton);}
    void deallocate();
  };


  /***
   * Example usage:
   *
   * class A : public Singleton<A> {
   *   A(Inaccessible);
   *   . . .
   *
   * Then in the implementation file:
   *
   *   A *Singleton<A>::singleton = 0;
   *
   *   A::A(Inaccessible) {
   *     . . .
   */
  template <typename T>
  class Singleton : public Base {
  protected:
    static Singleton<T> *singleton;

    // Restrict access to constructor and destructor
    Singleton() {
      if (singleton)
        CBANG_THROWS("There can be only one. . .instance of singleton "
               << typeid(T).name());

      singleton = this;
      SingletonDealloc::instance().add(singleton);
    }

    virtual ~Singleton() {singleton = 0;}

  public:
    inline static T &instance() {
      if (!singleton) new T(Inaccessible());

      T *ptr = dynamic_cast<T *>(singleton);
      if (!ptr) CBANG_THROWS("Invalid singleton, not of type "
                             << typeid(T).name());

      return *ptr;
    }
  };


  template <typename T> Singleton<T> *Singleton<T>::singleton = 0;
}

#endif // CBANG_SINGLETON_H
