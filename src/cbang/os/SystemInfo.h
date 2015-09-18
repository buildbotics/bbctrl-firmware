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

#ifndef CBANG_SYSTEM_INFO_H
#define CBANG_SYSTEM_INFO_H

#include "CPUID.h"

#include <cbang/StdTypes.h>
#include <cbang/SmartPointer.h>

#include <cbang/util/Singleton.h>
#include <cbang/util/Version.h>

#include <cbang/enum/ThreadsType.h>

#include <vector>
#include <utility>

namespace cb {
  class Info;

  class SystemInfo : public Singleton<SystemInfo>, public CPUID {
    ThreadsType threadsType;

  public:
    typedef enum {
      MEM_INFO_TOTAL,
      MEM_INFO_FREE,
    } memory_info_t;

    SystemInfo(Inaccessible);

    uint32_t getCPUCount() const;
    ThreadsType getThreadsType() {return threadsType;}

    uint64_t getMemoryInfo(memory_info_t type) const;
    uint64_t getTotalMemory() const {return getMemoryInfo(MEM_INFO_TOTAL);}
    uint64_t getFreeMemory() const {return getMemoryInfo(MEM_INFO_FREE);}

    static uint64_t getFreeDiskSpace(const std::string &path);

    Version getOSVersion() const;

    void add(Info &info);

  protected:
    void detectThreads();
  };
}

#endif // CBANG_SYSTEM_INFO_H
