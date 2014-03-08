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

#include "Condition.h"

#include "MutexPrivate.h"
#ifdef _WIN32
#include "Semaphore.h"
#endif

#include <cbang/Exception.h>
#include <cbang/Zap.h>

#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>
#include <cbang/util/SmartLock.h>

using namespace cb;

namespace cb {
  struct Condition::private_t {
#ifdef _WIN32
    Semaphore blockLock;
    Semaphore blockQueue;
    Mutex unblockLock;

    int gone;
    int blocked;
    int unblock;

    private_t() : blockLock("", 1), gone(0), blocked(0), unblock(0) {}

#else // pthreads
    pthread_cond_t cond;
#endif
  };
};


Condition::Condition() : p(new Condition::private_t) {
#ifndef _WIN32
  if (pthread_cond_init(&p->cond, 0))
    THROW("Failed to initialize condition");
#endif
}


Condition::~Condition() {
#ifndef _WIN32 // pthreads
  if (p) pthread_cond_destroy(&p->cond);
#endif

  zap(p);
}

void Condition::wait() {
  if (!isLocked()) THROW("Condition not locked!");

#ifdef _WIN32
  timedWait(-1);

#else // pthreads
  if (pthread_cond_wait(&p->cond, &Mutex::p->mutex))
    THROW("Failed to wait on condition");
#endif
}


bool Condition::timedWait(double timeout) {
  if (!isLocked()) THROW("Condition not locked!");

#ifdef _WIN32
  // Windows Server 2003 and Windows XP/2000 and eariler do not support
  // condition variables.  This implementation uses an algorithm described
  // in the pthreads-win32 source.
  //
  // See algorithm described here:
  //   ftp://sourceware.org/pub/pthreads-win32/sources/
  //     pthreads-w32-2-8-0-release/pthread_cond_wait.c
  //
  // And discussion here:
  //   http://blogs.msdn.com/oldnewthing/archive/2005/01/05/346888.aspx

  bool timedout = true;

  p->blockLock.wait();
  p->blocked++;
  p->blockLock.post();

  unlock();
  try {
    int signals = 0;
    int wasGone = 0;
    timedout = p->blockQueue.wait(timeout);

    {
      SmartLock lock(&p->unblockLock);

      signals = p->unblock;

      if (signals) {
        if (timedout) {
          if (p->blocked) p->blocked--;
          else p->gone++; // Spurious wakeup
        }

        if (!--p->unblock) {
          if (p->blocked) {
            p->blockLock.post();
            signals = 0;

          } else {
            wasGone = p->gone;
            p->gone = 0;
          }
        }

      } else if ((1 << 30) == ++p->gone) {
        // p->gone overflowed
        p->blockLock.wait();
        p->blocked -= p->gone;
        p->blockLock.post();
        p->gone = 0;
      }
    }

    if (signals == 1) {
      if (wasGone) while (wasGone--) p->blockQueue.wait(); // Spurious
      else p->blockLock.post();
    }

  } catch (...) {
    lock();
    throw;
  }

  lock();

  return !timedout;

#else // pthreads
  timeout += Timer::now(); // Convert to absolute time
  struct timespec t = Timer::toTimeSpec(timeout);

  return pthread_cond_timedwait(&p->cond, &Mutex::p->mutex, &t) == 0;
#endif
}


void Condition::signal(bool broadcast) {
#ifdef _WIN32
  int signals = 0;

  {
    SmartLock(&p->unblockLock);

    if (p->unblock) {
      if (!p->blocked) return;

      if (broadcast) {
        signals = p->blocked;
        p->unblock += signals;
        p->blocked = 0;

      } else {
        signals = 1;
        p->unblock++;
        p->blocked--;
      }

    } else if (p->blocked > p->gone) {
      p->blockLock.wait();

      p->blocked -= p->gone;
      p->gone = 0;

      if (broadcast) {
        signals = p->unblock = p->blocked;
        p->blocked = 0;

      } else {
        signals = p->unblock = 1;
        p->blocked--;
      }
    }
  }

  p->blockQueue.post(signals);

#else // pthreads
  if (broadcast) pthread_cond_broadcast(&p->cond);
  else pthread_cond_signal(&p->cond);
#endif
}
