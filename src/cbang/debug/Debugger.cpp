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

#include <cbang/debug/Debugger.h>

#include <cbang/debug/BacktraceDebugger.h>

#include <cbang/util/Singleton.h>
#include <cbang/util/SmartLock.h>

#include <stdexcept>

#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/stat.h>
#endif

using namespace std;
using namespace cb;

Mutex Debugger::lock;
Debugger *Debugger::singleton = 0;


Debugger &Debugger::instance() {
  SmartLock l(&lock);

  if (!singleton) {
    if (getenv("DISABLE_DEBUGGER")) singleton = new Debugger(); // Null debugger
#ifdef HAVE_CBANG_BACKTRACE
    else if (BacktraceDebugger::supported())
      singleton = new BacktraceDebugger();
#endif // HAVE_CBANG_BACKTRACE
    else singleton = new Debugger(); // Null debugger

    SingletonDealloc::instance().add(singleton);
  }

  return *singleton;
}


bool Debugger::printStackTrace(ostream &stream) {
  StackTrace trace;
  if (!instance().getStackTrace(trace)) return false;

  unsigned count = 0;
  bool skip = true;

  StackTrace::iterator it;
  for (it = trace.begin(); it != trace.end(); it++) {
    if (skip) {
      if (it->getFunction().find("cb::Debugger::printStackTrace"))
        skip = false;
    } else  stream << "\n  #" << ++count << ' ' << *it;
  }

  return true;
}


StackTrace Debugger::getStackTrace() {
  StackTrace trace;
  instance().getStackTrace(trace);
  return trace;
}


string Debugger::getExecutableName() {
#ifndef _WIN32
  char path[4096];
  ssize_t end;

  if ((end = readlink("/proc/self/exe", path, 4095)) == -1)
    throw runtime_error("Could not read link /proc/self/exe");

  path[end] = 0;

  struct stat buf;
  if (stat(path, &buf))
    throw runtime_error(string("Could not stat '") + path + "'");

  return (char *)path;

#else
  throw runtime_error("Not supported");
#endif
}


extern "C" {
  int cbang_print_stacktrace() {
    return Debugger::printStackTrace(cout);
  }
}


#ifdef DEBUGGER_TEST

#include <cbang/Exception.h>

#include <iostream>


void b(int x) {
  THROW("Test cause!");
}


void a(char *x) {
  try {
    b(10);
  } catch (Exception &e) {
    THROWC("Test exception!", e);
  }
}


int main(int argc, char *argv[]) {
  try {
    a("test");
  } catch (Exception &e) {
    cerr << "Exception: " << e << endl;
  }

  return 0;
}

#endif // DEBUGGER_TEST
