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

#include "BacktraceDebugger.h"

#ifdef HAVE_CBANG_BACKTRACE

#include <cbang/String.h>
#include <cbang/Zap.h>

#include <cbang/util/SmartLock.h>

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cxxabi.h>

#include <execinfo.h>
#include <bfd.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_VALGRIND
#include <valgrind/memcheck.h>
#endif

// NOTE Cannot throw cb::Exception here because of the dependency

using namespace std;
using namespace cb;

struct BacktraceDebugger::private_t {
  bfd *abfd;
  asymbol **syms;
};


BacktraceDebugger::BacktraceDebugger() :
  p(new private_t), initialized(false), enabled(true), parents(2) {
  memset(p, 0, sizeof(private_t));
}


BacktraceDebugger::~BacktraceDebugger() {}


bool BacktraceDebugger::supported() {
  return true;
}


bool BacktraceDebugger::getStackTrace(StackTrace &trace) {
  init(); // Might set enabled false
  if (!enabled) return false;

  void *stack[maxStack];
  int n = backtrace(stack, maxStack);

#ifdef VALGRIND_MAKE_MEM_DEFINED
  VALGRIND_MAKE_MEM_DEFINED(stack, n * sizeof(void *));
#endif // VALGRIND_MAKE_MEM_DEFINED

  //cerr << "backtrace() = " << n << endl;

  SmartLock lock(this);

  for (int i = 0; i < n; i++) {
    bfd_vma pc = (bfd_vma)stack[i];

    const char *filename = 0;
    const char *function = 0;
    unsigned line = 0;

    // Find section
    asection *section = 0;
    for (asection *s = p->abfd->sections; s; s = s->next) {
      if ((bfd_get_section_flags(p->abfd, s) & SEC_CODE) == 0)
        continue;

      bfd_vma vma = bfd_get_section_vma(p->abfd, s);
      if (pc < vma) {
        section = s->prev;
        break;
      }
    }

    if (section) {
      bfd_vma vma = bfd_get_section_vma(p->abfd, section);
      bfd_find_nearest_line(p->abfd, section, p->syms, pc - vma,
                            &filename, &function, &line);

#ifdef VALGRIND_MAKE_MEM_DEFINED
      if (filename) VALGRIND_MAKE_MEM_DEFINED(filename, strlen(filename));
      if (function) VALGRIND_MAKE_MEM_DEFINED(function, strlen(function));
      VALGRIND_MAKE_MEM_DEFINED(&line, sizeof(line));
#endif // VALGRIND_MAKE_MEM_DEFINED
    }

    // Filename
    if (!filename) filename = "";
    else if (0 <= parents) {
      const char *ptr = filename + strlen(filename);
      for (int j = 0; j <= parents; j++)
        while (filename < ptr && *--ptr != '/') continue;

      if (*ptr == '/') filename = ptr + 1;
    }

    // Function name
    char *demangled = 0;
    if (!function) function = "";
    else {
      int status = 0;
      demangled = abi::__cxa_demangle(function, 0, 0, &status);
      if (!status && demangled) function = demangled;
    }

    trace.push_back(StackFrame(stack[i], FileLocation(filename, line),
                               function));

    if (demangled) free(demangled);
  }

  return true;
}


void BacktraceDebugger::init() {
  if (!enabled || initialized) return;

  //cerr << "BacktraceDebugger::init()" << endl;

  try {
    bfd_init();

    executable = getExecutableName();

    if (!(p->abfd = bfd_openr(executable.c_str(), 0)))
      throw runtime_error(string("Error opening BDF in '") + executable + "'");

    if (bfd_check_format(p->abfd, bfd_archive))
      throw runtime_error("Not a BFD archive");

    char **matching;
    if (!bfd_check_format_matches(p->abfd, bfd_object, &matching)) {
      if (bfd_get_error() == bfd_error_file_ambiguously_recognized)
        free(matching);
      throw runtime_error("Does not match BFD object");
    }

    if ((bfd_get_file_flags(p->abfd) & HAS_SYMS) == 0)
      throw runtime_error("No symbols");

    long storage = bfd_get_symtab_upper_bound(p->abfd);
    if (storage <= 0) THROW("Invalid storage size");

    p->syms = (asymbol **)new char[storage];

    long count = bfd_canonicalize_symtab(p->abfd, p->syms);
    if (count < 0) throw runtime_error("Invalid symbol count");

#if 0
    asection *section;
    for (section = p->abfd->sections; section; section = section->next)
      cerr << bfd_get_section_name(p->abfd, section) << " "
           << bfd_get_section_vma(p->abfd, section) << " "
           << bfd_get_section_lma(p->abfd, section) << endl;
#endif

  } catch (const std::exception &e) {
    cerr << "Failed to initialize BFD for stack traces: " << e.what() << endl;
    enabled = false;
  }

  initialized = true;
}


void BacktraceDebugger::release() {
  if (p->abfd) bfd_close(p->abfd);
  if (p->syms) delete [] p->syms;
}


#else // HAVE_CBANG_BACKTRACE
bool cb::BacktraceDebugger::supported() {
  return false;
}
#endif // HAVE_CBANG_BACKTRACE
