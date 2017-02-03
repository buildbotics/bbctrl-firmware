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

#include "Semaphore.h"

#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/Zap.h>

#include <cbang/time/Timer.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else // _WIN32
// Posix semaphores
#include <fcntl.h> // For O_* constants
#include <semaphore.h>
#include <errno.h>
#endif // _WIN32

#ifdef __APPLE__
#include <uuid/uuid.h>
#endif

using namespace std;
using namespace cb;


namespace cb {
  struct Semaphore::private_t {
#ifdef _WIN32
    HANDLE sem;
#else // _WIN32
    sem_t *sem;
#ifdef __APPLE__
    char name2[39];
#endif
#endif // _WIN32
  };
}


Semaphore::Semaphore(const string &name, unsigned count, unsigned mode) :
  p(new private_t), name(name) {

#ifdef _WIN32
  // TODO use 'mode' to create SECURITY_ATTRIBUTES
  p->sem =
    CreateSemaphore(0, count, 1 << 30, isNamed() ? name.c_str(): 0);
  if (p->sem == 0)
    THROWS("Failed to create Semaphore: " << SysError());

#else // _WIN32
  if (isNamed()) {
    p->sem = sem_open(name.c_str(), O_CREAT, (mode_t)mode, count);
    if (p->sem == SEM_FAILED)
      THROWS("Failed to create Semaphore: " << SysError());

  } else {
#ifdef __APPLE__
    // unnamed semaphores are deprecated and unimplemented on osx
    uuid_t uuid;
    uuid_generate(uuid);
    p->name2[0] = '/';
    p->name2[1] = 's';
    uuid_unparse_lower(uuid, &(p->name2[2]));
    p->sem = sem_open(p->name2, O_CREAT, (mode_t)mode, count);
    if (p->sem == SEM_FAILED)
      THROWS("Failed to create Semaphore: " << SysError());

#else
    p->sem = new sem_t;
    if (sem_init(p->sem, 0, count))
      THROWS("Failed to initialize Semaphore: " << SysError());
#endif // __APPLE__
  }
#endif // _WIN32
}


Semaphore::~Semaphore() {
#ifdef _WIN32
  if (p->sem) CloseHandle(p->sem);

#else // _WIN32
#ifdef __APPLE__
  if (p->sem) {
    sem_close(p->sem);
    if (!isNamed())
      sem_unlink(p->name2);
  }
#else // __APPLE__
  if (isNamed()) {
    if (p->sem) sem_close(p->sem);

  } else if (p->sem) {
    sem_destroy(p->sem);
    zap(p->sem);
  }
#endif // __APPLE__
#endif // _WIN32

  zap(p);
}


bool Semaphore::wait(double timeout) const {
#ifdef _WIN32
  DWORD t = timeout < 0 ? INFINITE : (DWORD)(timeout * 1000);
  DWORD ret = WaitForSingleObject(p->sem, t);

  if (ret == WAIT_OBJECT_0) return true;
  if (ret == WAIT_TIMEOUT) return false;

#else // _WIN32
  int ret;
  if (timeout < 0) ret = sem_wait(p->sem);
  else if (timeout == 0) ret = sem_trywait(p->sem); // Can return EAGAIN
  else {
#ifdef __APPLE__
    // OS X does not support sem_timedwait() so we fake it
    Timer timer(true);

    while (true) {
      if ((ret = sem_trywait(p->sem)) != EAGAIN) break;

      double delta = timeout - timer.delta();
      if (delta <= 0) break; // Timedout (ret == EAGAIN)
      Timer::sleep(delta < 0.1 ? delta : 0.1);
    }

#else // __APPLE__
    timeout += Timer::now(); // Convert to absolute time
    struct timespec t = Timer::toTimeSpec(timeout);
    ret = sem_timedwait(p->sem, &t);
#endif // __APPLE__
  }

  if (ret == ETIMEDOUT || ret == EAGAIN) return false;
  else if (!ret) return true;
#endif // _WIN32

  THROWS("Wait on Semaphore failed: " << SysError());
}


void Semaphore::post(unsigned count) const {
  if (!count) return;

#ifdef _WIN32
  if (!ReleaseSemaphore(p->sem, count, 0))
    THROWS("Semaphore post failed: " << SysError());

#else // _WIN32
  while (count--)
    if (sem_post(p->sem))
      THROWS("Semaphore post failed: " << SysError());
#endif // _WIN32
}
