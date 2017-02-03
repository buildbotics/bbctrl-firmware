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

#include "Condition.h"

#include "MutexPrivate.h"
#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/Zap.h>

#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>
#include <cbang/util/SmartLock.h>

using namespace cb;

namespace cb {
  struct Condition::private_t {
#ifdef _WIN32
    // Number of waiting threads.
    int waitersCount;

    // Serialize access to waitersCount.
    CRITICAL_SECTION waitersCountLock;

    // Semaphore used to queue threads waiting for the condition.
    HANDLE sema;

    // An auto-reset event used by the broadcast/signal thread to wait for all
    // waiting thread(s) to wake up and be released from the semaphore.
    HANDLE waitersDone;

    // Keeps track of whether we were broadcasting or signaling.  This allows
    // us to optimize the code if we're just signaling.
    bool wasBroadcast;

    private_t() : waitersCount(0), wasBroadcast(false) {}

#else // pthreads
    pthread_cond_t cond;
#endif
  };
};


Condition::Condition() : p(new Condition::private_t) {
#ifdef _WIN32
  p->sema = CreateSemaphore(0, 0, 0x7fffffff, 0);
  InitializeCriticalSection(&p->waitersCountLock);
  p->waitersDone = CreateEvent(0, FALSE, FALSE, 0);

#else
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
  // See algorithm 3.4 described here:
  //   http://www.cs.wustl.edu/~schmidt/win32-cv-1.html

  // Avoid race conditions.
  EnterCriticalSection(&p->waitersCountLock);
  p->waitersCount++;
  LeaveCriticalSection(&p->waitersCountLock);

  // This call atomically releases the mutex and waits on the semaphore until
  // signal() is called by another thread.
  DWORD t = timeout < 0 ? INFINITE : (DWORD)(timeout * 1000);
  DWORD ret = SignalObjectAndWait(Mutex::p->h, p->sema, t, FALSE);

  // Reacquire lock to avoid race conditions.
  EnterCriticalSection(&p->waitersCountLock);

  // We're no longer waiting...
  p->waitersCount--;

  // Check to see if we're the last waiter after a broadcast.
  bool lastWaiter = p->wasBroadcast && p->waitersCount == 0;

  LeaveCriticalSection(&p->waitersCountLock);

  // If we're the last waiter thread during this particular broadcast then let
  // all the other threads proceed.
  if (lastWaiter)
    // This call atomically signals the waitersDone event and waits until
    // it can acquire the mutex.  This is required to ensure fairness.
    SignalObjectAndWait(p->waitersDone, Mutex::p->h, INFINITE, FALSE);
  else
    // Always regain the mutex since that's the guarantee we give our callers.
    WaitForSingleObject(Mutex::p->h, INFINITE);

  // Process return code from SignalObjectAndWait() above.
  if (ret == WAIT_TIMEOUT) return true;
  else if (ret == WAIT_FAILED) THROWS("Wait failed: " << SysError());
  else if (ret == WAIT_ABANDONED) // Lock still acquired
    LOG_WARNING("Wait Abandoned, Mutex owner terminated");

  return false;

#else // pthreads
  timeout += Timer::now(); // Convert to absolute time
  struct timespec t = Timer::toTimeSpec(timeout);

  return pthread_cond_timedwait(&p->cond, &Mutex::p->mutex, &t) == 0;
#endif
}


void Condition::signal(bool broadcast) {
  SmartLock lock(this);

#ifdef _WIN32
  if (broadcast) {
    // This is needed to ensure that waitersCount and wasBroadcast are
    // consistent relative to each other.
    EnterCriticalSection(&p->waitersCountLock);

    if (p->waitersCount > 0) {
      // We are broadcasting, even if there is just one waiter.  Record that we
      // are broadcasting, which helps optimize wait() for the non-broadcast
      // case.
      p->wasBroadcast = true;

      // Wake up all the waiters atomically.
      ReleaseSemaphore(p->sema, p->waitersCount, 0);

      LeaveCriticalSection(&p->waitersCountLock);

      // Wait for all the awakened threads to acquire the counting semaphore.
      WaitForSingleObject(p->waitersDone, INFINITE);

      // This assignment is okay, even without the waitersCountLock held
      // because no other waiter threads can wake up to access it.
      p->wasBroadcast = false;

    } else LeaveCriticalSection(&p->waitersCountLock);

  } else { // Signal
    EnterCriticalSection(&p->waitersCountLock);
    bool haveWaiters = p->waitersCount > 0;
    LeaveCriticalSection(&p->waitersCountLock);

    // If there aren't any waiters, then this is a no-op.
    if (haveWaiters) ReleaseSemaphore(p->sema, 1, 0);
  }

#else // pthreads
  if (broadcast) pthread_cond_broadcast(&p->cond);
  else pthread_cond_signal(&p->cond);
#endif
}
