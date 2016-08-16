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

#include "SysError.h"

#ifdef _WIN32
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#else
#include <string.h>
#include <errno.h>
#endif

using namespace std;
using namespace cb;


SysError::SysError(int code) : code(code ? code : get()) {}


string SysError::toString() const {
#ifdef _WIN32
  // TODO revisit this for unicode builds
  LPSTR buffer = 0;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS, 0, (DWORD)code,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0,
                0);
  string str = buffer;

  LocalFree(buffer);

  return str;

#else // _WIN32
#if _POSIX_C_SOURCE >= 200112L
  char buffer[4096];

#ifdef _GNU_SOURCE
  return strerror_r(code, buffer, 4096);

#else // _GNU_SOURCE
  if (strerror_r(code, buffer, 4096)) return "Unknown error";
  return buffer;
#endif // !_GNU_SOURCE

#else // _POSIX_C_SOURCE >= 200112L
  return strerror(code);

#endif // _POSIX_C_SOURCE < 200112L
#endif // !_WIN32
}


void SysError::set(int code) {
#ifdef _WIN32
  SetLastError((DWORD)code);
#else
  errno = code;
#endif
}


int SysError::get() {
#ifdef _WIN32
  return (int)GetLastError();
#else
  return errno;
#endif
}
