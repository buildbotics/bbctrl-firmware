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

#include "Thread.h"

#include "ThreadLocalStorage.h"
#include "SysError.h"
#include "SystemUtilities.h"

#include <cbang/Exception.h>

#include <cbang/util/DefaultCatch.h>
#include <cbang/log/Logger.h>
#include <cbang/debug/Debugger.h>

#include <exception>

#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#else // pthreads
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#endif

#ifdef __APPLE__
#include <sched.h>
#endif // __APPLE__

#ifdef HAVE_VALGRIND_DRD_H
#include <valgrind/drd.h>
#endif

using namespace std;
using namespace cb;

namespace cb {
  struct Thread::private_t {
#ifdef _WIN32
    HANDLE h;
    DWORD id;

#else // pthreads
    pthread_t thread;
#endif
  };
};


#ifdef _WIN32
static DWORD WINAPI start_func(LPVOID t) {
  ((Thread *)t)->starter();
  return 0;
}

#else // pthreads
static void *start_func(void *t) {
  ((Thread *)t)->starter();
  return 0;
}
#endif

ThreadLocalStorage<Thread *> Thread::threads;


Thread::Thread(bool destroy) :
  p(new Thread::private_t), state(THREAD_STOPPED), shutdown(false),
  destroy(destroy), id(getNextID()), exitStatus(0) {

  threads.set(this);

#ifdef HAVE_VALGRIND_DRD_H
  DRD_IGNORE_VAR((const state_t &)state);
  DRD_IGNORE_VAR((const bool &)shutdown);
#endif // HAVE_VALGRIND_DRD_H
}


Thread::~Thread() {
  if (state != THREAD_STOPPED)
    LOG_ERROR(Exception(SSTR("Thread " << getID()
                             << " deallocated while still active")));

  if (p) {
    delete p;
    p = 0;
  }
}


void Thread::start() {
  if (state != THREAD_STOPPED) join();

  state = THREAD_STARTING;
  exitStatus = 0;
  shutdown = false;

#ifdef _WIN32
  p->h = CreateThread(0, 0, start_func, this, 0, &p->id);
  int error = !p->h;

#else // pthreads
  int error = pthread_create(&p->thread, 0, start_func, this);
#endif

  if (error) {
    state = THREAD_STOPPED;

    string msg = "Unknown error";

#ifdef _WIN32
    msg = SysError().toString();

#else
    switch (error) {
    case EAGAIN: msg = "Insufficient resources"; break;
    case EINVAL: msg = "Invalid setting"; break;
    case EPERM: msg = "Permission denied"; break;
    }
#endif

    THROWS("Error creating thread: " << msg);
  }
}


void Thread::stop() {
  shutdown = true;
}


void Thread::join() {
  if (state == THREAD_STOPPED) return;

  if (!shutdown) stop();
  wait();
}


void Thread::wait() {
  if (state == THREAD_STOPPED) return;

#ifdef _WIN32
  WaitForSingleObject(p->h, INFINITE);
  CloseHandle(p->h);

#else // pthreads
  pthread_join(p->thread, 0);
#endif

  state = THREAD_STOPPED;
}


void Thread::cancel() {
  stop();

#ifdef _WIN32
  if (TerminateThread(p->h, 0)) state = THREAD_DONE;

#else // pthreads
  if (pthread_cancel(p->thread) == 0) state = THREAD_DONE;
#endif
}


void Thread::kill(int signal) {
#ifdef _WIN32
  TerminateThread(p->h, -1);
#else
  pthread_kill(p->thread, signal);
#endif
}


void Thread::yield() {
#ifdef _WIN32
  SwitchToThread();
#elif defined(__APPLE__)
  sched_yield();
#else
  pthread_yield();
#endif
}


uint64_t Thread::self() {
#ifdef _WIN32
  return (uint64_t)GetCurrentThreadId();
#else
  return (uint64_t)pthread_self();
#endif
}


Thread &Thread::current() {
  return *threads.get();
}


void Thread::starter() {
  state = THREAD_RUNNING;

  try {
    Logger::instance().setThreadID(getID());
    LOG_INFO(5, "Started thread " << getID() << " on PID "
             << SystemUtilities::getPID());
    run();
    done();
    return;

  } CATCH(LOG_ERROR_LEVEL, ": In thread " << id);

  exitStatus = -1;
  done();
}


void Thread::done() {
  state = THREAD_DONE;

  if (destroy) {
    state = THREAD_STOPPED;

#ifdef _WIN32
    CloseHandle(p->h);
#endif

    try {
      delete this;
    } CATCH_ERROR;
  }
}
