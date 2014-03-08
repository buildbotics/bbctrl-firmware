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

#ifndef MUTEX_H
#define MUTEX_H

#include <cbang/StdTypes.h>

#include <cbang/util/Lockable.h>
#include <cbang/util/NonCopyable.h>

namespace cb {
  /// Mutual exclusion class
  class Mutex : public Lockable, public NonCopyable {
  protected:
    struct private_t;
    private_t *p;

    mutable uint64_t locked;

  public:
    Mutex();
    ~Mutex();

    bool isLocked() const {return locked;}

    /**
     * Aquire this lock.  Will block the current thread until the lock is
     * obtained.
     */
    bool lock(double timeout = -1) const;

    /**
     * Release the lock.  Assumes the the lock is held.
     * See SmartLock for an exception safe way to unlock Mutexs.
     */
    void unlock() const;

    /**
     * Try to aquire this lock.
     * @return False if the lock is not immediately available.
     */
    bool tryLock() const;
  };
}

#endif // MUTEX_H
