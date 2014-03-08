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

#include <cbang/os/Mutex.h>

#include "MutexPrivate.h"

#include <cbang/Exception.h>

#include "SysError.h"

#include <cbang/util/ID.h>
#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>

#ifndef _WIN32
#include <errno.h> // For ETIMEDOUT and EBUSY
#endif

using namespace cb;


Mutex::Mutex() : p(new Mutex::private_t), locked(0) {
#ifdef _WIN32
  if (!(p->h = CreateMutex(0, false, 0)))
    THROW("Failed to initialize mutex");

#else // pthreads
  if (pthread_mutexattr_init(&p->attr))
    THROW("Failed to initialize mutex attribute");

  if (pthread_mutexattr_settype(&p->attr, PTHREAD_MUTEX_RECURSIVE))
    THROW("Failed to set mutex recursive");

  if (pthread_mutex_init(&p->mutex, &p->attr))
    THROW("Failed to initialize mutex");
#endif
}


Mutex::~Mutex() {
  if (p) {
#ifdef _WIN32
    CloseHandle(p->h);

#else // pthreads
    pthread_mutex_destroy(&p->mutex);
    pthread_mutexattr_destroy(&p->attr);
#endif

    delete p;
    p = 0;
  }
}


bool Mutex::lock(double timeout) const {
#ifdef _WIN32
  DWORD t = timeout < 0 ? INFINITE : (DWORD)(timeout * 1000);
  DWORD ret = WaitForSingleObject(p->h, t);
  if (ret == WAIT_TIMEOUT) return false;
  if (ret == WAIT_FAILED)
    THROWS("Wait failed: " << SysError());
  if (ret == WAIT_ABANDONED) // Lock still aquired
    LOG_WARNING("Wait Abandoned, Mutex owner terminated");

#else // pthreads
  int ret;

  if (timeout == 0) {
    ret = pthread_mutex_trylock(&p->mutex);

    if (ret == EBUSY) return false;
    if (ret) THROWS("Mutex " << ID((uint64_t)this) << " trylock failed: "
                    << SysError(ret));

  } else if (timeout < 0) {
    ret = pthread_mutex_lock(&p->mutex);

    if (ret) THROWS("Mutex " << ID((uint64_t)this) << " lock failed: "
                    << SysError(ret));

  } else {
#ifdef __APPLE__
    // OS X does not support pthread_mutex_timedlock() so we fake it
    int ret;
    Timer timer(true);

    while (true) {
      if ((ret = pthread_mutex_trylock(&p->mutex)) != EBUSY) break;

      double delta = timeout - timer.delta();
      if (delta <= 0) return false; // Timedout
      Timer::sleep(delta < 0.1 ? delta : 0.1);
    }

#else // __APPLE__
    struct timespec ts = Timer::toTimeSpec(Timer::now() + timeout);
    ret = pthread_mutex_timedlock(&p->mutex, &ts);

    if (ret == ETIMEDOUT) return false;
#endif // __APPLE__

    if (ret) THROWS("Mutex " << ID((uint64_t)this) << " timedlock failed: "
                    << SysError(ret));
  }
#endif // _WIN32

  locked++;
  return true;
}


void Mutex::unlock() const {
  if (!locked) THROWS("Mutex " << ID((uint64_t)this) << " was not locked");

  locked--;

  int ret = 0;

#ifdef _WIN32
  if (ReleaseMutex(p->h)) return;
#else // pthreads
  if ((ret = pthread_mutex_unlock(&p->mutex)) == 0) return;
#endif // _WIN32

  locked++;
  THROWS("Mutex " << ID((uint64_t)this) << " unlock failed: " << SysError(ret));
}


bool Mutex::tryLock() const {
  return lock(0);
}
