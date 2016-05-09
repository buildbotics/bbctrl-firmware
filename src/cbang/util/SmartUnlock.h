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

#ifndef CBANG_SMART_UNLOCK_H
#define CBANG_SMART_UNLOCK_H

#include <cbang/Exception.h>

#include <cbang/util/SmartFunctor.h>
#include <cbang/util/Lockable.h>
#include <cbang/log/Logger.h>

namespace cb {
  /**
   * This class is like a smart pointer for Lockables. It guarantees that
   * the Lockable gets relocked when the SmartUnlock goes out of scope.  This
   * makes unlocking safe when exceptions may be thrown.
   */
  class SmartUnlock : public SmartConstFunctor<SmartUnlock> {
    const Lockable *lock;

  public:
    SmartUnlock(const Lockable *lock, bool alreadyUnlocked = false) :
      Super(this, &SmartUnlock::close, false), lock(lock) {
      if (!alreadyUnlocked) lock->unlock();
      setEngaged(true);
    }

  protected:
    void close() const {lock->lock();}
  };
}

#endif // CBANG_SMART_UNLOCK_H
