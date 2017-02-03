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

#include "DynamicLibrary.h"

#include "SysError.h"

#include <cbang/Exception.h>
#include <cbang/Zap.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else
#include <dlfcn.h>
#endif

using namespace std;
using namespace cb;


bool DynamicLibrary::enabled = true;


struct DynamicLibrary::private_t {
#ifdef _WIN32
  HMODULE handle;
#else
  void *handle;
#endif
};


DynamicLibrary::DynamicLibrary(const string &path) :
  path(path), pri(new private_t) {

  if (!enabled) THROW("DynamicLibrary disabled globally");

#ifdef _WIN32
  pri->handle = LoadLibrary(path.c_str());
  if (!pri->handle)
    THROWS("Failed to open dynamic library '" << path << "': " << SysError());

#else
  dlerror(); // Clear errors

  pri->handle = dlopen(path.c_str(), RTLD_LAZY);
  if (!pri->handle)
    THROWS("Failed to open dynamic library '" << path << "': " << dlerror());
#endif
}


DynamicLibrary::~DynamicLibrary() {
#ifdef _WIN32
  if (pri->handle) CloseHandle(pri->handle);

#else
  if (pri->handle) dlclose(pri->handle);
#endif

  zap(pri);
}


void *DynamicLibrary::getSymbol(const string &name) {
#ifdef _WIN32
  void *symbol = (void *)GetProcAddress(pri->handle, name.c_str());
  if (!symbol)
    THROWS("Failed to load dynamic symbol '" << name << "' from library '"
           << path << "': " << SysError());

#else
  dlerror(); // Clear errors

  void *symbol = dlsym(pri->handle, name.c_str());

  char *err = dlerror();
  if (err)
    THROWS("Failed to load dynamic symbol '" << name << "' from library '"
           << path << "': " << err);
#endif

  return symbol;
}
