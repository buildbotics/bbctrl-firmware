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

#include "SignalManager.h"

#include "SystemUtilities.h"
#include "SystemInfo.h"

#include <cbang/Exception.h>
#include <cbang/Zap.h>

#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>
#include <cbang/util/SmartLock.h>

#include <signal.h>

using namespace std;
using namespace cb;


namespace {
  void sig_handler(int sig) {SignalManager::instance().signal(sig);}
}

#ifndef _MSC_VER
#include <unistd.h>
#include <string.h>
#endif

struct SignalManager::private_t {
#ifndef _MSC_VER
  sigset_t currentSet;
  sigset_t nextSet;
  bool linuxThreads;
  bool dirty;
#endif // _MSC_VER
};


SignalManager::SignalManager(Inaccessible) :
  pri(new private_t), enabled(false) {
#ifndef _MSC_VER
  sigemptyset(&pri->currentSet);
  sigemptyset(&pri->nextSet);

  pri->linuxThreads =
    SystemInfo::instance().getThreadsType() == ThreadsType::LINUX_THREADS;

  pri->dirty = false;
#endif // _MSC_VER
}


SignalManager::~SignalManager() {
  setEnabled(false);
  zap(pri);
}


void SignalManager::setEnabled(bool x) {
  if (enabled == x) return;
  enabled = x;

#ifndef _MSC_VER
  // Threaded implementation is safer with LinuxThreads, found in
  // kernels <= 2.4, as it guarantees that signals are received only once and
  // avoids some bugs.

  if (enabled) start();
  else {
    stop();
    Condition::signal();
    join();
  }
#endif
}


void SignalManager::addHandler(int sig, SignalHandler *handler) {
  SmartLock lock(this);

  if (handler) {
    handlers_t::iterator it = handlers.find(sig);
    if (it != handlers.end())
      THROWS("Signal " << sig << " already has handler.");
  }

#ifdef _MSC_VER
  ::signal(sig, sig_handler);

#else
  block(sig); // Block in main & inherited threads
  sigaddset(&pri->nextSet, sig);
  pri->dirty = true;
  Condition::signal();
#endif

  if (handler) handlers[sig] = handler;
}


void SignalManager::removeHandler(int sig) {
  SmartLock lock(this);

  handlers.erase(sig);

#ifdef _MSC_VER
  ::signal(sig, SIG_DFL);

#else
  unblock(sig); // Unblock in main & inherited threads
  sigdelset(&pri->nextSet, sig);
  pri->dirty = true;
  Condition::signal();
#endif
}


void SignalManager::signal(int sig) {
  LOG_INFO(1, "Caught signal " << signalString(sig) << "(" << sig
           << ") on PID " << SystemUtilities::getPID());

  if (!enabled) return;

  handlers_t::iterator it = handlers.find(sig);
  if (it != handlers.end()) it->second->handleSignal(sig);
}


void SignalManager::run() {
#ifndef _MSC_VER
  while (true) {
    update();
    {
      SmartLock lock(this);
      timedWait(0.1);
    }
    if (shouldShutdown()) break;
  }

  restore();
#endif // _MSC_VER
}


void SignalManager::update() {
#ifndef _MSC_VER
  sigset_t oldSet;
  {
    SmartLock lock(this);

    if (!pri->dirty) return;
    pri->dirty = false;

    // Get new signal set
    oldSet = pri->currentSet;
    pri->currentSet = pri->nextSet;
  }

  struct sigaction action;
  memset(&action, 0, sizeof(action));

  // Update signal handlers
  //
  // A handler is needed so that interruptable system calls get
  // interrupted. This works because SA_RESTART is not set for
  // these handlers which causes interuptable system calls to
  // interupt rather than restart when the handler is completed.
  // See signal(7) 'Interruption of System Calls and Library
  // Functions by Signal Handlers'.
  for (int sig = 1; sig < 32; sig++) {
    // Added signal
    if (sigismember(&pri->currentSet, sig) && !sigismember(&oldSet, sig)) {
      action.sa_handler = sig_handler;
      sigaction(sig, &action, 0);
    }

    // Removed signal
    if (!sigismember(&pri->currentSet, sig) && sigismember(&oldSet, sig)) {
      action.sa_handler = SIG_DFL;
      sigaction(sig, &action, 0);
    }
  }

  // Make sure these signals are unblocked for this thread
  pthread_sigmask(SIG_UNBLOCK, &pri->currentSet, 0);

#endif // _MSC_VER
}


void SignalManager::restore() {
#ifndef _MSC_VER
  // Restore default handlers
  struct sigaction action;

  memset(&action, 0, sizeof(action));
  action.sa_handler = SIG_DFL;

  for (int sig = 1; sig < 32; sig++)
    if (sigismember(&pri->currentSet, sig)) sigaction(sig, &action, 0);
#endif // _MSC_VER
}


void SignalManager::block(int sig) {
#ifdef _MSC_VER
  THROW("Not supported on Windows");

#else
  sigset_t mask;
  pthread_sigmask(SIG_SETMASK, 0, &mask); // Get current signal set
  sigaddset(&mask, sig); // Add signal
  pthread_sigmask(SIG_SETMASK, &mask, 0); // Set updated signal set
#endif
}


void SignalManager::unblock(int sig) {
#ifdef _MSC_VER
  THROW("Not supported on Windows");

#else
  sigset_t mask;
  pthread_sigmask(SIG_SETMASK, 0, &mask); // Get current signal set
  sigdelset(&mask, sig); // Remove signal
  pthread_sigmask(SIG_SETMASK, &mask, 0); // Set updated signal set
#endif
}


const char *SignalManager::signalString(int sig) {
  switch (sig) {
  case SIGABRT: return "SIGABRT";
  case SIGFPE: return "SIGFPE";
  case SIGILL: return "SIGILL";
  case SIGINT: return "SIGINT";
  case SIGSEGV: return "SIGSEGV";
  case SIGTERM: return "SIGTERM";
  case SIGHUP: return "SIGHUP";
  case SIGQUIT: return "SIGQUIT";
#ifndef _MSC_VER
  case SIGKILL: return "SIGKILL";
  case SIGPIPE: return "SIGPIPE";
  case SIGALRM: return "SIGALRM";
  case SIGUSR1: return "SIGUSR1";
  case SIGUSR2: return "SIGUSR2";
  case SIGCHLD: return "SIGCHLD";
  case SIGCONT: return "SIGCONT";
  case SIGSTOP: return "SIGSTOP";
  case SIGBUS: return "SIGBUS";
#endif
  default: return "UNKNOWN";
  }
}
