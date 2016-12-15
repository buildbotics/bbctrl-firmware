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

#ifdef _WIN32

#include "Win32Utilities.h"
#include "SysError.h"

#include <cbang/Exception.h>

#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>
#include <io.h>

using namespace cb;


namespace cb {
  namespace Win32Utilities {
    void setInherit(int fd, bool inherit) {
      setInherit((void *)_get_osfhandle(fd), inherit);
    }


    void setInherit(void *handle, bool inherit) {
      if (handle == INVALID_HANDLE_VALUE) THROW("Invalid handle");

      DWORD flags = inherit ? HANDLE_FLAG_INHERIT : 0;
      if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, flags))
        THROWS("Failed to clear pipe inherit flag: " << SysError());
    }


    unsigned priorityToClass(ProcessPriority priority) {
      switch (priority) {
      case ProcessPriority::PRIORITY_INHERIT: return 0;
      case ProcessPriority::PRIORITY_NORMAL: return NORMAL_PRIORITY_CLASS;
      case ProcessPriority::PRIORITY_IDLE: return IDLE_PRIORITY_CLASS;
      case ProcessPriority::PRIORITY_LOW: return BELOW_NORMAL_PRIORITY_CLASS;
      case ProcessPriority::PRIORITY_HIGH: return ABOVE_NORMAL_PRIORITY_CLASS;
      case ProcessPriority::PRIORITY_REALTIME: return REALTIME_PRIORITY_CLASS;
      default: THROWS("Invalid priority: " << priority);
      }
    }
  };
};

#endif // _WIN32
