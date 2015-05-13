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

#include "ServerApplication.h"

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/SignalManager.h>
#include <cbang/os/ProcessLock.h>
#include <cbang/os/Subprocess.h>

#include <cbang/log/Logger.h>
#include <cbang/time/Time.h>
#include <cbang/time/Timer.h>
#include <cbang/util/DefaultCatch.h>

#include <signal.h>
#include <sstream>

using namespace cb;
using namespace cb;
using namespace std;


ServerApplication::ServerApplication(const string &name,
                                     hasFeature_t hasFeature) :
  Application(name, hasFeature), restartChild(false), lifeline(0) {

  if (!hasFeature(FEATURE_SERVER)) return;

  typedef OptionAction<ServerApplication> Action;

  cmdLine.add("chdir", 0, new Action(this, &ServerApplication::chdirAction),
              "Change directory before starting server.  All files opened "
              "after this point, such as the configuration file, must have "
              "paths relative to the new directory."
              )->setType(Option::STRING_TYPE);

  if (hasFeature(FEATURE_LIFELINE))
    cmdLine.add("lifeline", 0, new OptionActionSet<uint64_t>(lifeline),
                "The application will watch for this process ID and exit if it "
                "goes away.  Usually the calling process' PID."
                )->setType(Option::INTEGER_TYPE);

  options.pushCategory("Process Control");
#ifndef _WIN32
  options.add("run-as", "Run as specified user");
  options.add("fork", "Run in background.")->setDefault(false);
#endif

  options.add("respawn", "Run the application as a child process and respawn "
              "if it is killed or exits.")->setDefault(false);
  options.add("pid", "Create PID file.")->setDefault(false);
  options.add("pid-file", "Name of PID file.")->setDefault(name + ".pid");
  options.add("child", "Disable 'daemon', 'fork', 'pid' and 'respawn' options. "
              "Also defaults 'log-to-screen' to false. Used internally."
              )->setDefault(false);

  options.add("daemon", 0, new Action(this, &ServerApplication::daemonAction),
              "Short for --pid --service --respawn --log=''"
#ifndef _WIN32
              " --fork"
#endif
              )->setDefault(false);
  options.popCategory();
}


bool ServerApplication::_hasFeature(int feature) {
  if (feature == FEATURE_SERVER) return true;
  return Application::_hasFeature(feature);
}


int ServerApplication::init(int argc, char *argv[]) {
  // Hide child's output
  if (hasFeature(FEATURE_SERVER) && options["child"].toBoolean())
    ;//options["log-to-screen"].setDefault(false);

  int ret = Application::init(argc, argv);
  if (ret == -1) return -1;

  if (!hasFeature(FEATURE_SERVER)) return ret;

  if (!options["child"].toBoolean()) {
#ifndef _WIN32
    if (options["fork"].toBoolean()) {
      // Note, all threads must be stopped before daemonize() forks and
      // SignalManager runs in a thread.
      SignalManager::instance().setEnabled(false);
      SystemUtilities::daemonize();
      SignalManager::instance().setEnabled(true);
    }
#endif

    if (options["pid"].toBoolean()) {
      // Try to acquire an exclusive lock
      new ProcessLock(options["pid-file"], 10); // OK to leak
      LOG_INFO(1, "Acquired exclusive lock on " << options["pid-file"]);
    }
  }

#ifndef _WIN32
  try {
    if (options["run-as"].hasValue()) {
      LOG_INFO(1, "Switching to user " << options["run-as"]);
      SystemUtilities::setUser(options["run-as"]);
    }
  } CBANG_CATCH_ERROR;
#endif

  if (!options["child"].toBoolean() && options["respawn"].toBoolean()) {
#ifndef _WIN32
    // Child restart handler
    SignalManager::instance().addHandler(SIGUSR1, this);
#endif

    // Setup child arguments
    vector<string> args;
    args.push_back(argv[0]);
    args.push_back((char *)"--child");

    if (hasFeature(FEATURE_LIFELINE)) {
      args.push_back((char *)"--lifeline");
      args.push_back(String(SystemUtilities::getPID()));
    }

    args.insert(args.end(), argv + 1, argv + argc);

    unsigned lastRespawn = 0;
    unsigned respawnCount = 0;

    while (!shouldQuit()) {
      // Check respawn rate
      unsigned now = Time::now();
      if (now - lastRespawn < 60) {
        if (++respawnCount == 5) {
          LOG_ERROR("Respawning too fast. Exiting.");
          break;
        }
      } else respawnCount = 0;
      lastRespawn = now;

      // Spawn child
      Subprocess child;
      child.exec(args, Subprocess::NULL_STDOUT | Subprocess::NULL_STDERR);

      uint64_t killTime = 0;
      while (child.isRunning()) {
        if (!killTime && (restartChild || shouldQuit())) {
          LOG_INFO(1, "Shutting down child at PID=" << child.getPID());

          child.interrupt();
          restartChild = false;
          killTime = Time::now() + 300; // Give it 5 mins to shutdown
        }

        if (killTime && killTime < Time::now()) {
          LOG_INFO(1, "Child failed to shutdown cleanly, killing");

          killTime = 0;
          child.kill();
        }

        Timer::sleep(0.1);
      }

      LOG_INFO(1, "Child exited with return code " << child.getReturnCode());
    }

    exit(0);
  }

  return ret;
}


bool ServerApplication::shouldQuit() const {
  if (!quit && lostLifeline())
    const_cast<ServerApplication *>(this)->requestExit();

  return Application::shouldQuit();
}


bool ServerApplication::lostLifeline() const {
  if (!hasFeature(FEATURE_LIFELINE)) return false;

  if (!lifeline || SystemUtilities::pidAlive(lifeline)) return false;

  LOG_INFO(1, "Lost lifeline PID " << lifeline << ", exiting");

  return true;
}


int ServerApplication::chdirAction(Option &option) {
  SystemUtilities::chdir(option.toString());
  return 0;
}


int ServerApplication::daemonAction() {
  if (!options["child"].toBoolean()) {
    if (options["daemon"].toBoolean()) {
      options["respawn"].set(true);
#ifndef _WIN32
      options["fork"].set(true);
#endif
      options["pid"].set(true);
      if (hasFeature(FEATURE_PROCESS_CONTROL))
        options["service"].set(true);
      options["log"].unset();

    } else LOG_WARNING("deamon=false has no effect");
  }

  return 0;
}


void ServerApplication::handleSignal(int sig) {
#ifndef _WIN32
  // Restart child
  if (hasFeature(FEATURE_SERVER) && sig == SIGUSR1) {
    restartChild = true;
    return;
  }
#endif

  Application::handleSignal(sig);
}
