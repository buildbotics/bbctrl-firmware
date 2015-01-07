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

#include "Exception.h"
#include "String.h"

#include <iomanip>

#include <cbang/debug/Debugger.h>

using namespace std;
using namespace cb;


#ifdef DEBUG
bool Exception::enableStackTraces = true;
#else
bool Exception::enableStackTraces = false;
#endif

bool Exception::printLocations = true;
unsigned Exception::causePrintLevel = 10;


ostream &Exception::print(ostream &stream, unsigned level) const {
  if (code) stream << code << ": ";

  stream << message;

  if (printLocations && !location.isEmpty())
    stream << "\n       At: " << location;

  if (!trace.isNull()) {
    unsigned count = 0;
    unsigned i = 0;

    StackTrace::iterator it;
    for (it = trace->begin(); it != trace->end(); it++, i++) {
      if (i < 3 && (it->getFunction().find("Debugger") ||
                    it->getFunction().find("Exception")))
        continue;
      else  stream << "\n  #" << ++count << ' ' << *it;
    }
  }

  if (!cause.isNull()) {
    stream << endl;

    if (level > causePrintLevel) {
      stream << "Aborting exception dump due to cause print level limit! "
        "Increase Exception::causePrintLevel to see more.";

    } else {
      stream << "Caused by: ";
      cause->print(stream, level + 1);
    }
  }

  return stream;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern "C" void convert_win32_exception(unsigned x, EXCEPTION_POINTERS *e){
  const char *msg;
  switch (e->ExceptionRecord->ExceptionCode) {
  case EXCEPTION_ACCESS_VIOLATION: msg = "Exception access violation"; break;
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: msg = "Array bounds exceeded"; break;
  case EXCEPTION_BREAKPOINT: msg = "Breakpoint"; break;
  case EXCEPTION_DATATYPE_MISALIGNMENT: msg = "Datatype misalignment"; break;
  case EXCEPTION_FLT_DENORMAL_OPERAND:
    msg = "Floating-point denormal operand"; break;
  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    msg = "Floating-point divide by zero"; break;
  case EXCEPTION_FLT_INEXACT_RESULT:
    msg = "Floating-point inexact result"; break;
  case EXCEPTION_FLT_INVALID_OPERATION:
    msg = "Floating-point invalid operation"; break;
  case EXCEPTION_FLT_OVERFLOW: msg = "Floating-point overflow"; break;
  case EXCEPTION_FLT_STACK_CHECK: msg = "Floating-point stack check"; break;
  case EXCEPTION_FLT_UNDERFLOW: msg = "Floating-point underflow"; break;
  case EXCEPTION_ILLEGAL_INSTRUCTION: msg = "Illegal instruction"; break;
  case EXCEPTION_IN_PAGE_ERROR: msg = "In page error"; break;
  case EXCEPTION_INT_DIVIDE_BY_ZERO: msg = "Tnteger divide by zero"; break;
  case EXCEPTION_INT_OVERFLOW: msg = "Integer overflow"; break;
  case EXCEPTION_INVALID_DISPOSITION: msg = "Invalid disposition"; break;
  case EXCEPTION_NONCONTINUABLE_EXCEPTION:
    msg = "Noncontinuable exception"; break;
  case EXCEPTION_PRIV_INSTRUCTION: msg = "Private instruction"; break;
  case EXCEPTION_SINGLE_STEP: msg = "Single step"; break;
  case EXCEPTION_STACK_OVERFLOW: msg = "Stack overflow"; break;
  default: msg = "Unknown"; break;
  }

  THROWS("Win32: 0x" << hex << x << ": " << msg);
}
#endif // _WIN32

void Exception::init() {
#ifdef _WIN32
  _set_se_translator(convert_win32_exception);
#endif

#ifdef HAVE_DEBUGGER
  if (enableStackTraces) {
    trace = new StackTrace();
    Debugger::instance().getStackTrace(*trace);
  }
#endif
}
