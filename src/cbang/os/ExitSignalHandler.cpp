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

#include "ExitSignalHandler.h"

#include <cbang/Exception.h>
#include <cbang/log/Logger.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SignalManager.h>

#include <signal.h>

using namespace std;
using namespace cb;


#ifdef _MSC_VER
#define USE_MSC_VER_CONSOLE_CTRL_HANDLER
#endif

#ifdef USE_MSC_VER_CONSOLE_CTRL_HANDLER
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>
static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType) {
  LOG_WARNING("Console control signal " << dwCtrlType << " on PID "
              << SystemUtilities::getPID());

  try {
    ExitSignalHandler &handler = ExitSignalHandler::instance();

    switch (dwCtrlType) {
    case CTRL_C_EVENT: handler.signal(SIGINT); break;
    case CTRL_BREAK_EVENT: handler.signal(SIGQUIT); break;
    case CTRL_CLOSE_EVENT: handler.signal(SIGTERM); break;
    case CTRL_LOGOFF_EVENT: handler.signal(SIGHUP); break;
    case CTRL_SHUTDOWN_EVENT: handler.signal(SIGTERM); break;
    default: break;
    }

  } catch (const Exception &e) {return 0;}

  return 1;
}
#endif


void ExitSignalHandler::catchExitSignals() {
  SignalManager &signalManager = SignalManager::instance();

#ifdef USE_MSC_VER_CONSOLE_CTRL_HANDLER
  SetConsoleCtrlHandler(ConsoleCtrlHandler, true);

#else
#ifndef _MSC_VER
  signalManager.addHandler(SIGHUP, this);
  signalManager.addHandler(SIGQUIT, this);
#endif
  signalManager.addHandler(SIGTERM, this);
  signalManager.addHandler(SIGINT, this);
#endif

  signalManager.setEnabled(true);
}


void ExitSignalHandler::ignoreExitSignals() {
#ifdef USE_MSC_VER_CONSOLE_CTRL_HANDLER
  SetConsoleCtrlHandler(ConsoleCtrlHandler, false);

#else
  SignalManager &signalManager = SignalManager::instance();

#ifndef _MSC_VER
  signalManager.removeHandler(SIGHUP);
  signalManager.removeHandler(SIGQUIT);
#endif
  signalManager.removeHandler(SIGTERM);
  signalManager.removeHandler(SIGINT);
#endif
}


void ExitSignalHandler::handleSignal(int sig) {
  if (1 < ++count) {
    ignoreExitSignals();
    LOG_WARNING("Next signal will force exit");

  } else LOG_INFO(1, "Exiting, please wait. . .");
}
