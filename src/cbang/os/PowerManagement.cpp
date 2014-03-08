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

#include "PowerManagement.h"

#include "DynamicLibrary.h"

#include <cbang/Exception.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/util/DefaultCatch.h>
#include <cbang/time/Time.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

#else
typedef unsigned Window;
typedef struct {
  Window window;
  int state;
  int kind;
  unsigned long til_or_since;
  unsigned long idle;
  unsigned long eventMask;
} XScreenSaverInfo;

typedef void *(*XOpenDisplay_t)(char *);
typedef XScreenSaverInfo *(*XScreenSaverAllocInfo_t)();
typedef void (*XScreenSaverQueryInfo_t)(void *, unsigned, XScreenSaverInfo *);
typedef Window (*XDefaultRootWindow_t)(void *);
#endif

#include <string.h>

using namespace cb;
using namespace std;

SINGLETON_DECL(PowerManagement);


struct PowerManagement::private_t {
#if defined(__APPLE__)
  IOPMAssertionID displayAssertionID;
  IOPMAssertionID systemAssertionID;
#elif defined(_WIN32)
#else
  bool initialized;
  void *display;
  Window root;
  DynamicLibrary *xssLib;
  XScreenSaverInfo *info;
#endif
};


PowerManagement::PowerManagement(Inaccessible) :
  lastBatteryUpdate(0), systemOnBattery(false), systemHasBattery(false),
  lastIdleSecondsUpdate(0), idleSeconds(0), systemSleepAllowed(true),
  displaySleepAllowed(true), pri(new private_t) {
  memset(pri, 0, sizeof(private_t));
}


bool PowerManagement::onBattery() {
  updateBatteryInfo();
  return systemOnBattery;
}


bool PowerManagement::hasBattery() {
  updateBatteryInfo();
  return systemHasBattery;
}


unsigned PowerManagement::getIdleSeconds() {
  updateIdleSeconds();
  return idleSeconds;
}


void PowerManagement::allowSystemSleep(bool x) {
  if (systemSleepAllowed == x) return;

#if defined(_WIN32)
  systemSleepAllowed = x;
  SetThreadExecutionState(ES_CONTINUOUS |
                          (systemSleepAllowed ? 0 :
                           (ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)) |
                          (displaySleepAllowed ? 0 : ES_DISPLAY_REQUIRED));

#elif defined(__APPLE__)
  IOPMAssertionID &assertionID = pri->systemAssertionID;

  if (!x) {
    if (IOPMAssertionCreateWithName(kIOPMAssertionTypeNoIdleSleep,
                                    kIOPMAssertionLevelOn,
                                    CFSTR("FAHClient"), &assertionID) ==
        kIOReturnSuccess) systemSleepAllowed = false;
    else assertionID = 0;

  } else if (assertionID) {
    IOPMAssertionRelease(assertionID);
    assertionID = 0;
    systemSleepAllowed = true;
  }
#else

  // TODO
#endif
}


void PowerManagement::allowDisplaySleep(bool x) {
  if (displaySleepAllowed == x) return;

#if defined(_WIN32)
  displaySleepAllowed = x;
  SetThreadExecutionState(ES_CONTINUOUS |
                          (systemSleepAllowed ? 0 :
                           (ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED)) |
                          (displaySleepAllowed ? 0 : ES_DISPLAY_REQUIRED));

#elif defined(__APPLE__)
  IOPMAssertionID &assertionID = pri->displayAssertionID;

  if (!x) {
    if (IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
                                    kIOPMAssertionLevelOn,
                                    CFSTR("FAHClient"), &assertionID) ==
        kIOReturnSuccess) displaySleepAllowed = false;
    else assertionID = 0;

  } else if (assertionID) {
    IOPMAssertionRelease(assertionID);
    assertionID = 0;
    displaySleepAllowed = false;
  }
#else

  // TODO
#endif
}


void PowerManagement::updateIdleSeconds() {
  // Avoid checking this too often
  if (Time::now() <= lastIdleSecondsUpdate) return;
  lastIdleSecondsUpdate = Time::now();

  idleSeconds = 0;

#if defined(_WIN32)
  LASTINPUTINFO lif;
  lif.cbSize = sizeof(LASTINPUTINFO);
  GetLastInputInfo(&lif);

  idleSeconds = (GetTickCount() - lif.dwTime) / 1000; // Convert from ms.

#elif defined(__APPLE__)
  io_iterator_t iter = 0;
  if (IOServiceGetMatchingServices(kIOMasterPortDefault,
                                   IOServiceMatching("IOHIDSystem"), &iter) ==
      KERN_SUCCESS) {

    io_registry_entry_t entry = IOIteratorNext(iter);
    if (entry)  {
      CFMutableDictionaryRef dict = NULL;
      if (IORegistryEntryCreateCFProperties(entry, &dict, kCFAllocatorDefault,
                                            0) == KERN_SUCCESS) {
        CFNumberRef obj =
          (CFNumberRef)CFDictionaryGetValue(dict, CFSTR("HIDIdleTime"));

        if (obj) {
          int64_t nanoseconds = 0;
          if (CFNumberGetValue(obj, kCFNumberSInt64Type, &nanoseconds))
            idleSeconds =
              (unsigned)(nanoseconds / 1000000000); // Convert from ns.
        }
        CFRelease(dict);
      }
      IOObjectRelease(entry);
    }
    IOObjectRelease(iter);
  }

#else
  // NOTE We use dynamic library access to avoid a direct dependency on X11
  try {
    if (!pri->initialized) {
      pri->initialized = true;

      // Get display
      DynamicLibrary xLib("libX11.so");
      pri->display = xLib.accessSymbol<XOpenDisplay_t>("XOpenDisplay")(0);
      if (!pri->display) return;

      // Get default root window
      pri->root = xLib.accessSymbol<XDefaultRootWindow_t>
        ("XDefaultRootWindow")(pri->display);

      // Get XScreensaver lib
      pri->xssLib = new DynamicLibrary("libXss.so");

      // Allocate XScreenSaverInfo
      pri->info = pri->xssLib->accessSymbol<XScreenSaverAllocInfo_t>
        ("XScreenSaverAllocInfo")();
    }

    if (!pri->display || !pri->info) return;

    // Query idle state
    pri->xssLib->accessSymbol<XScreenSaverQueryInfo_t>
      ("XScreenSaverQueryInfo")(pri->display, pri->root, pri->info);

    idleSeconds = pri->info->idle / 1000; // Convert from ms.

  } catch (...) {} // Ignore
#endif
}


void PowerManagement::updateBatteryInfo() {
  // Avoid checking this too often
  if (Time::now() <= lastBatteryUpdate) return;
  lastBatteryUpdate = Time::now();

  systemOnBattery = systemHasBattery  = false;

#if defined(_WIN32)
  SYSTEM_POWER_STATUS status;

  if (GetSystemPowerStatus(&status)) {
    systemOnBattery = status.ACLineStatus == 0;
    systemHasBattery = (status.BatteryFlag & 128) == 0;
  }

#elif defined(__APPLE__)
  CFTypeRef info = IOPSCopyPowerSourcesInfo();
  if (info) {
    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (list) {
      CFIndex count = CFArrayGetCount(list);
      if (count > 0) systemHasBattery = true;
      for (CFIndex i=0; i < count; i++) {
        CFTypeRef item = CFArrayGetValueAtIndex(list, i);
        CFDictionaryRef battery = IOPSGetPowerSourceDescription(info, item);
        CFTypeRef value =
          CFDictionaryGetValue(battery, CFSTR(kIOPSPowerSourceStateKey));
        systemOnBattery = !CFEqual(value, CFSTR(kIOPSACPowerValue));
        if (systemOnBattery) break;
      }

      CFRelease(list);
    }

    CFRelease(info);
  }

#elif defined(__FreeBSD__)
    // TODO

#else
  try {
    const char *sysBase = "/sys/class/power_supply";
    const char *procBase = "/proc/acpi/ac_adapter";

    bool useSys = SystemUtilities::exists(sysBase);
    bool useProc = !useSys && SystemUtilities::exists(procBase);

    if (!useSys && !useProc) return;

    string base = useSys ? sysBase : procBase;

    // Has battery
    const char *batNames[] = {"/BAT", "/BAT0", "/BAT1", 0};
    for (int i = 0; !systemHasBattery && batNames[i]; i++)
      systemHasBattery = SystemUtilities::exists(base + batNames[i]);

    // On battery
    if (systemHasBattery) {
      if (useSys) {
        const char *acNames[] = { "/AC0/online", "/AC/online", 0};

        for (int i = 0; acNames[i]; i++) {
          string path = base + acNames[i];

          if (SystemUtilities::exists(path))
            systemOnBattery = String::trim(SystemUtilities::read(path)) == "0";
        }

      } else if (useProc) {
        string path = base + "/AC0/state";

        if (SystemUtilities::exists(path)) {
          vector<string> tokens;
          String::tokenize(SystemUtilities::read(path), tokens);

          if (tokens.size() == 2 && tokens[0] == "state")
            systemOnBattery = tokens[1] != "on-line";
        }
      }
    }
  } CBANG_CATCH_ERROR;
#endif
}
