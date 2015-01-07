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

#include "RWLock.h"
#include "Thread.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#else // pthreads
#include <pthread.h>
#endif

using namespace cb;

namespace cb {
  struct RWLock::private_t {
#ifdef _WIN32
    SRWLOCK lock;
    bool exclusive;

#else // pthreads
    pthread_rwlock_t lock;
#endif
  };
};


RWLock::RWLock() : p(new RWLock::private_t) {
#ifdef _WIN32
  InitializeSRWLock(&p->lock);

#else // pthreads
  if (pthread_rwlock_init(&p->lock, 0))
    THROW("Failed to initialize rwlock");
#endif
}


RWLock::~RWLock() {
  if (p) {
#ifdef _WIN32

#else // pthreads
    pthread_rwlock_destroy(&p->lock);
#endif

    delete p;
    p = 0;
  }
}


void RWLock::readLock() const {
  LOG_DEBUG(5, "readLock() " << Thread::self());

#ifdef _WIN32
  // TODO this only works in Vista+
  AcquireSRWLockShared(&p->lock);
  p->exclusive = false;
#else // pthreads
  pthread_rwlock_rdlock(&p->lock);
#endif

  LOG_DEBUG(5, "readLock() " << Thread::self() << " acquired");
}


void RWLock::writeLock() const {
  LOG_DEBUG(5, "writeLock() " << Thread::self());

#ifdef _WIN32
  // TODO this only works in Vista+
  AcquireSRWLockExclusive(&p->lock);
  p->exclusive = true;
#else // pthreads
  pthread_rwlock_wrlock(&p->lock);
#endif

  LOG_DEBUG(5, "writeLock() " << Thread::self() << " acquired");
}


void RWLock::unlock() const {
  LOG_DEBUG(5, "unlock() " << Thread::self());

#ifdef _WIN32
  // TODO this only works in Vista+
  if (p->exclusive) ReleaseSRWLockExclusive(&p->lock);
  else ReleaseSRWLockShared(&p->lock);
#else // pthreads
  pthread_rwlock_unlock(&p->lock);
#endif
}


bool RWLock::tryReadLock() const {
#ifdef _WIN32
  THROW("Not supported in Windows");
#else // pthreads
  return pthread_rwlock_tryrdlock(&p->lock) == 0;
#endif
}


bool RWLock::tryWriteLock() const {
#ifdef _WIN32
  THROW("Not supported in Windows");
#else // pthreads
  return pthread_rwlock_trywrlock(&p->lock) == 0;
#endif
}
