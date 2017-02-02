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

#include "SystemInfo.h"
#include "PowerManagement.h"

#include <cbang/Info.h>
#include <cbang/SStream.h>
#include <cbang/String.h>

#include <cbang/os/Thread.h>
#include <cbang/os/SystemUtilities.h>

#include <cbang/log/Logger.h>
#include <cbang/util/HumanSize.h>

#include <boost/filesystem/operations.hpp>

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>

#include <mach/vm_statistics.h>
#include <mach/mach_types.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>

#else // !_MSC_VER && !__APPLE__
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif

#ifndef _MSC_VER
#include <unistd.h>
#endif

using namespace cb;
using namespace std;

namespace fs = boost::filesystem;


SystemInfo::SystemInfo(Inaccessible) {
  detectThreads();
}


uint32_t SystemInfo::getCPUCount() const {
#if defined(_MSC_VER)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;

#elif defined(__APPLE__) || defined(__FreeBSD__)
  int nm[2];
  size_t length = 4;
  uint32_t count;

  nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
  sysctl(nm, 2, &count, &length, 0, 0);

  if (count < 1) {
    nm[1] = HW_NCPU;
    sysctl(nm, 2, &count, &length, 0, 0);
    if (count < 1) count = 1;
  }

  return count;

#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}


uint64_t SystemInfo::getMemoryInfo(memory_info_t type) const {
#if defined(_MSC_VER)
  MEMORYSTATUSEX info;

  info.dwLength = sizeof(MEMORYSTATUSEX);
  GlobalMemoryStatusEx(&info);

  switch (type) {
  case MEM_INFO_TOTAL: return (uint64_t)info.ullTotalPhys;
  case MEM_INFO_FREE: return (uint64_t)info.ullAvailPhys;
  }

#elif defined(__APPLE__) || defined(__FreeBSD__)
  switch (type) {
  case MEM_INFO_TOTAL: {
    int64_t memory;
    int mib[2];
    size_t length = 2;

    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    length = sizeof(int64_t);

    if (!sysctl(mib, 2, &memory, &length, 0, 0)) return memory;
    break;
  }

  case MEM_INFO_FREE: {
    vm_size_t page;
    vm_statistics_data_t stats;
    mach_msg_type_number_t count = sizeof(stats) / sizeof(natural_t);
    mach_port_t port = mach_host_self();

    if (host_page_size(port, &page) == KERN_SUCCESS &&
        host_statistics(port, HOST_VM_INFO, (host_info_t)&stats, &count) ==
        KERN_SUCCESS)
      return (uint64_t)stats.free_count * page;
    break;
  }
  }

#else
  struct sysinfo info;
  if (!sysinfo(&info))
    switch (type) {
    case MEM_INFO_TOTAL: return (uint64_t)info.totalram * info.mem_unit;
    case MEM_INFO_FREE: return (uint64_t)info.freeram * info.mem_unit;
    }
#endif

  return 0;
}


uint64_t SystemInfo::getFreeDiskSpace(const string &path) {
  fs::space_info si;

  try {
    si = fs::space(path);

  } catch (const fs::filesystem_error &e) {
    THROWS("Could not get disk space at '" << path << "': " << e.what());
  }

  return si.available;
}


Version SystemInfo::getOSVersion() const {
#if defined(_MSC_VER)
  OSVERSIONINFO info;
  ZeroMemory(&info, sizeof(info));
  info.dwOSVersionInfoSize = sizeof(info);
  if (!GetVersionEx(&info)) THROWS("Failed to get Windows version");

  return Version((uint8_t)info.dwMajorVersion, (uint8_t)info.dwMinorVersion);

#elif defined(__APPLE__)
  SInt32 major;
  SInt32 minor;
  Gestalt(gestaltSystemVersionMajor, &major);
  Gestalt(gestaltSystemVersionMinor, &minor);

  return Version((uint8_t)major, (uint8_t)minor);

#else
  struct utsname i;

  uname(&i);
  string release = i.release;
  size_t dot = release.find('.');
  uint8_t major = String::parseU32(release.substr(0, dot));
  uint8_t minor = String::parseU32(release.substr(dot + 1));

  return Version(major, minor);
#endif
}


void SystemInfo::add(Info &info) {
  const char *category = "System";

  info.add(category, "CPU", getCPUBrand());
  info.add(category, "CPU ID",
           SSTR(getCPUVendor() << " Family " << getCPUFamily() << " Model "
                << getCPUModel() << " Stepping " << getCPUStepping()));
  info.add(category, "CPUs", String(getCPUCount()));

  info.add(category, "Memory", HumanSize(getTotalMemory()).toString() + "B");
  info.add(category, "Free Memory",
           HumanSize(getFreeMemory()).toString() + "B");
  info.add(category, "Threads", getThreadsType().toString());

  Version osVersion = getOSVersion();
  info.add(category, "OS Version",
           SSTR((unsigned)osVersion.getMajor() << '.'
                << (unsigned)osVersion.getMinor()));

  info.add(category, "Has Battery",
           String(PowerManagement::instance().hasBattery()));
  info.add(category, "On Battery",
           String(PowerManagement::instance().onBattery()));
}




void SystemInfo::detectThreads() {
  // Threads type
#ifdef _MSC_VER
  threadsType = ThreadsType::WINDOWS_THREADS;

#elif __linux__
  // Test for LinuxThreads which has a different PID for each thread
  struct TestThread : public Thread {
    unsigned pid;
    TestThread() : pid(0) {}
    void run() {pid = SystemUtilities::getPID();}
  };

  TestThread testThread;
  testThread.start();
  testThread.join();

  if (testThread.pid != SystemUtilities::getPID())
    threadsType = ThreadsType::LINUX_THREADS;
  else threadsType = ThreadsType::POSIX_THREADS;

#else // __linux__
  // Assume POSIX
  threadsType = ThreadsType::POSIX_THREADS;
#endif
}
