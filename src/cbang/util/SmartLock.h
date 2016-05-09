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

#ifndef CBANG_SMART_LOCK_H
#define CBANG_SMART_LOCK_H

#include <cbang/Exception.h>

#include <cbang/util/SmartFunctor.h>
#include <cbang/util/Lockable.h>
#include <cbang/log/Logger.h>

namespace cb {
  /**
   * This class is like a smart pointer for Lockables. It guarantees that
   * the Lockable gets unlocked when the SmartLock goes out of scope.  This
   * makes locking safe when exceptions may be thrown.
   */
  class SmartLock : public SmartConstFunctor<Lockable> {
  public:
    SmartLock(const Lockable *lock, double timeout = -1,
              bool alreadyLocked = false) :
      Super(lock, &Lockable::unlock, false) {

      if (!alreadyLocked && !lock->lock(timeout)) {
        if (timeout == -1) CBANG_THROWS("Failed to acquire lock");
        else CBANG_THROWS("Failed to acquire lock, timeout was " << timeout);

      } else setEngaged(true);
    }
  };
}

#endif // CBANG_SMART_LOCK_H
