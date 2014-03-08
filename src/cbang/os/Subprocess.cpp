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

#include "Subprocess.h"

#include "SystemUtilities.h"
#include "Win32Utilities.h"
#include "SysError.h"
#include "SystemInfo.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/Zap.h>

#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>

#include <string.h> // For memset()

#ifdef _WIN32
#include <windows.h>

#else // _WIN32
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif // _WIN32

#include <boost/version.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#if BOOST_VERSION < 104400
#define BOOST_CLOSE_HANDLE true
#else
#define BOOST_CLOSE_HANDLE io::close_handle
#endif

namespace io = boost::iostreams;

using namespace cb;
using namespace std;


struct Subprocess::private_t {
#ifdef _WIN32
  PROCESS_INFORMATION pi;
  STARTUPINFO si;

  HANDLE stdInPipe[2];
  HANDLE stdOutPipe[2];
  HANDLE stdErrPipe[2];

#else // _WIN32
  pid_t pid;

  int stdInPipe[2];
  int stdOutPipe[2];
  int stdErrPipe[2];
#endif // _WIN32

  typedef io::stream<io::file_descriptor_sink> sink_t;
  typedef io::stream<io::file_descriptor_source> source_t;

  sink_t *stdIn;
  source_t *stdOut;
  source_t *stdErr;
};


Subprocess::Subprocess() :
  p(new private_t), running(false), wasKilled(false), dumpedCore(false),
  signalGroup(false),
  returnCode(0) {
  memset(p, 0, sizeof(private_t));
}


Subprocess::~Subprocess() {
  closeStdIn();
  closeStdOut();
  closeStdErr();
  zap(p);
}


bool Subprocess::isRunning() {
  if (running) wait(true);
  return running;
}


uint64_t Subprocess::getPID() const {
#ifdef _WIN32
  return p->pi.dwProcessId;

#else // _WIN32
  return p->pid;
#endif // _WIN32
}


ostream &Subprocess::getStdIn() const {
  if (!p->stdIn) THROW("Subprocess stdIn stream not available");
  return *p->stdIn;
}


istream &Subprocess::getStdOut() const {
  if (!p->stdOut) THROW("Subprocess stdOut stream not available");
  return *p->stdOut;
}


istream &Subprocess::getStdErr() const {
  if (!p->stdErr) THROW("Subprocess stdErr stream not available");
  return *p->stdErr;
}


void Subprocess::closeStdIn() {
  zap(p->stdIn);
}


void Subprocess::closeStdOut() {
  zap(p->stdOut);
}


void Subprocess::closeStdErr() {
  zap(p->stdErr);
}


#ifdef _WIN32
static void createPipe(HANDLE *pipe, bool toChild) {
  // Setup security attributes for pipe inheritance
  SECURITY_ATTRIBUTES sAttrs;
  memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
  sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
  sAttrs.bInheritHandle = TRUE;
  sAttrs.lpSecurityDescriptor = 0;

  if (!CreatePipe(&pipe[0], &pipe[1], &sAttrs, 0))
    THROWS("Failed to create pipe: " << SysError());

  // Don't inherit other handle
  if (!SetHandleInformation(pipe[toChild ? 1 : 0], HANDLE_FLAG_INHERIT, 0))
    THROWS("Failed to clear pipe inherit flag: " << SysError());
}

static HANDLE OpenNUL() {
  // Setup security attributes for pipe inheritance
  SECURITY_ATTRIBUTES sAttrs;
  memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
  sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
  sAttrs.bInheritHandle = TRUE;
  sAttrs.lpSecurityDescriptor = 0;

  return CreateFile("NUL", GENERIC_WRITE, 0, &sAttrs, OPEN_EXISTING, 0, 0);
}
#endif


void Subprocess::exec(const vector<string> &_args, unsigned flags,
                      ProcessPriority priority) {
  if (isRunning()) THROW("Subprocess already running");

  returnCode = 0;
  wasKilled = dumpedCore = false;

  LOG_DEBUG(3, "Executing: " << assemble(_args));

  // Don't redirect stderr if merging
  if (flags & MERGE_STDOUT_AND_STDERR) flags &= ~REDIR_STDERR;

  // Don't redirect streams if closing
  if (flags & NULL_STDOUT) flags &= ~REDIR_STDOUT;
  if (flags & NULL_STDERR) flags &= ~REDIR_STDERR;

  // Send signals to whole process group
  // TODO this is kind of hackish
  signalGroup = flags & CREATE_PROCESS_GROUP;

  // Close any open handles
  closeHandles();

  try {
#ifdef _WIN32
    // Create pipes
    if (flags & REDIR_STDIN) createPipe(p->stdInPipe, true);
    if (flags & REDIR_STDOUT) createPipe(p->stdOutPipe, false);
    if (flags & REDIR_STDERR) createPipe(p->stdErrPipe, false);

    // Clear PROCESS_INFORMATION
    memset(&p->pi, 0, sizeof(PROCESS_INFORMATION));

    // Configure STARTUPINFO
    memset(&p->si, 0, sizeof(STARTUPINFO));
    p->si.cb = sizeof(STARTUPINFO);
    p->si.hStdInput =
      (flags & REDIR_STDIN) ? p->stdInPipe[0] : GetStdHandle(STD_INPUT_HANDLE);
    p->si.hStdOutput =
      (flags & REDIR_STDOUT) ? p->stdOutPipe[1] :
      GetStdHandle(STD_OUTPUT_HANDLE);
    p->si.hStdError = (flags & REDIR_STDERR) ? p->stdErrPipe[1] :
      ((flags & MERGE_STDOUT_AND_STDERR) ? p->stdOutPipe[1] :
       GetStdHandle(STD_ERROR_HANDLE));
    p->si.dwFlags |= STARTF_USESTDHANDLES;

    if (flags & NULL_STDOUT) p->si.hStdOutput = OpenNUL();
    if (flags & NULL_STDERR) p->si.hStdError = OpenNUL();

    // Hide window
    if (flags & W32_HIDE_WINDOW) {
      p->si.dwFlags |= STARTF_USESHOWWINDOW;
      p->si.wShowWindow = SW_HIDE;
    }

    // Setup environment
    const char *env = 0; // Use parents environment
    vector<char> newEnv;

    if (!StringMap::empty()) {
      StringMap copy;

      if (!(flags & CLEAR_ENVIRONMENT)) {
        // Copy environment
        LPTCH envStrs = GetEnvironmentStrings();
        if (envStrs) {
          const char *ptr = envStrs;
          do {
            const char *equal = strchr(ptr, '=');

            if (equal) copy[string(ptr, equal - ptr)] = string(equal + 1);

            ptr += strlen(ptr) + 1;

          } while (*ptr);

          FreeEnvironmentStrings(envStrs);
        }
      }

      // Overwrite with Subprocess vars
      copy.insert(StringMap::begin(), StringMap::end());

      // Load into vector
      StringMap::const_iterator it;
      for (it = copy.begin(); it != copy.end(); it++) {
        string s = it->first + '=' + it->second;
        newEnv.insert(newEnv.end(), s.begin(), s.end());
        newEnv.push_back(0); // Terminate string
      }
      newEnv.push_back(0); // Terminate block

      env = &newEnv[0];

    } else if (flags & CLEAR_ENVIRONMENT) {
      newEnv.push_back(0);
      newEnv.push_back(0);
      env = &newEnv[0];
    }

    // Working directory
    const char *dir = 0; // Default to current directory
    string wd = this->wd;
    if (!wd.empty()) {
      if (!SystemUtilities::isAbsolute(wd))
        wd = SystemUtilities::makeAbsolute(wd);
      dir = wd.c_str();
    }

    // Prepare command
    string command = assemble(_args);
    if (flags & SHELL) command = "cmd.exe /c " + command;

    // Creation flags
    DWORD cFlags = 0;
    if (flags & CREATE_PROCESS_GROUP) cFlags |= CREATE_NEW_PROCESS_GROUP;

    // Priority
    cFlags |= Win32Utilities::priorityToClass(priority);

    // Start process
    if (!CreateProcess(0, (LPSTR)command.c_str(), 0, 0, TRUE, cFlags,
                       (LPVOID)env, dir, &p->si, &p->pi))
      THROWS("Failed to create process with: " << command << ": "
             << SysError());

#else // _WIN32
    // Convert args
    vector<char *> args;
    for (unsigned i = 0; i < _args.size(); i++)
      args.push_back((char *)_args[i].c_str());
    args.push_back(0); // Sentinal

    // Create pipes
    if (((flags & REDIR_STDIN) && pipe(p->stdInPipe)) ||
        ((flags & REDIR_STDOUT) && pipe(p->stdOutPipe)) ||
        ((flags & REDIR_STDERR) && pipe(p->stdErrPipe)))
      THROWS("Failed to create pipes: " << SysError());

    p->pid = fork();

    if (!p->pid) { // Child

      // Process group
      if (flags & CREATE_PROCESS_GROUP) setpgid(0, 0);

      // Configure pipes
      if (flags & REDIR_STDIN) {
        close(p->stdInPipe[1]); // Close write end
        if (dup2(p->stdInPipe[0], 0) != 0) // Move to stdIn
          perror("Moving stdin descriptor");
      }

      if (flags & REDIR_STDOUT) {
        close(p->stdOutPipe[0]); // Close read end
        if (dup2(p->stdOutPipe[1], 1) != 1) // Move to stdOut
          perror("Moving stdout descriptor");
      }

      if (flags & MERGE_STDOUT_AND_STDERR)
        if (dup2(1, 2) != 2) perror("Copying stdout to stderr");

      if (flags & REDIR_STDERR) {
        close(p->stdErrPipe[0]); // Close read end
        if (dup2(p->stdErrPipe[1], 2) != 2) // Move to stdErr
          perror("Moving stderr descriptor");
      }

      if ((flags & NULL_STDOUT) || (flags & NULL_STDERR)) {
        int fd = open("/dev/null", O_WRONLY);

        if (fd != -1) {
          if (flags & NULL_STDOUT) dup2(fd, 1);
          if (flags & NULL_STDERR) dup2(fd, 2);

        } else {
          if (flags & NULL_STDOUT) close(1);
          if (flags & NULL_STDERR) close(2);
        }
      }

      // Priority
      SystemUtilities::setPriority(priority);

      // Working directory
      if (wd != "") SystemUtilities::chdir(wd);

      // Setup environment
      if (flags & CLEAR_ENVIRONMENT) SystemUtilities::clearenv();

      for (iterator it = begin(); it != end(); it++)
        SystemUtilities::setenv(it->first, it->second);

      SysError::clear();

      if (flags & SHELL) execvp(args[0], &args[0]);
      else execv(args[0], &args[0]);

      // Execution failed
      string errorStr = "Failed to execute: " + String::join(_args);
      perror(errorStr.c_str());
      exit(-1);

    } else if (p->pid == -1)
      THROWS("Failed to spawn subprocess: " << SysError());
#endif // _WIN32

  } catch (...) {
    closeHandles();
    throw;
  }

  // Create pipe streams
  if (flags & REDIR_STDIN) {
    p->stdIn = new private_t::sink_t(p->stdInPipe[1], BOOST_CLOSE_HANDLE);
    p->stdInPipe[1] = 0; // Don't close this end later
  }

  if (flags & REDIR_STDOUT) {
    p->stdOut = new private_t::source_t(p->stdOutPipe[0], BOOST_CLOSE_HANDLE);
    p->stdOutPipe[0] = 0; // Don't close this end later
  }

  if (flags & REDIR_STDERR) {
    p->stdErr = new private_t::source_t(p->stdErrPipe[0], BOOST_CLOSE_HANDLE);
    p->stdErrPipe[0] = 0; // Don't close this end later
  }

#ifndef _WIN32
  closePipes(); // Close pipe ends
#endif

  running = true;
}


void Subprocess::exec(const string &command, unsigned flags,
                      ProcessPriority priority) {
  vector<string> args;
  parse(command, args);
  exec(args, flags, priority);
}


void Subprocess::interrupt() {
  if (!running) THROW("Process not running!");
#ifdef _WIN32
  if (GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, p->pi.dwProcessId)) return;

#else // _WIN32
  int err;
  if (signalGroup) err = ::killpg((pid_t)getPID(), SIGINT);
  else err = ::kill((pid_t)getPID(), SIGINT);

  if (!err) return;
#endif // _WIN32

  THROWS("Failed to interrupt process " << getPID() << ": " << SysError());
}


void Subprocess::kill(bool nonblocking) {
  if (!running) THROW("Process not running!");

  SystemUtilities::killPID(getPID(), signalGroup);
  wasKilled = true;
  wait(nonblocking);
}


int Subprocess::wait(bool nonblocking) {
  if (!running) return returnCode;

  try {
    int flags = 0;
    running =
      !SystemUtilities::waitPID(getPID(), &returnCode, nonblocking, &flags);

    if (flags & SystemUtilities::PROCESS_SIGNALED) wasKilled = true;
    if (flags & SystemUtilities::PROCESS_DUMPED_CORE) dumpedCore = true;

  } catch (...) {
#ifndef _WIN32
    // Linux Threads, used in 2.4 kernels, uses a different process for each
    // thread.  Because of this you cannot reap the child process from a
    // thread other than the one the child process was started from.  This
    // causes waitpid() to fail when called from another thread.
    // As a work around we just ignore the error and hope the original thread
    // will reap the process later.
    if (SystemInfo::instance().getThreadsType() ==
        ThreadsType::LINUX_THREADS && errno == ECHILD) return 0;
#endif // _WIN32

    closeHandles();
    running = false;
    throw;
  }

  if (!running) closeHandles();

  return returnCode;
}


int Subprocess::waitFor(double interrupt, double kill) {
  const double delta = 0.25;

  double remain = interrupt;
  while (isRunning())
    if (interrupt && remain < delta) {
      Timer::sleep(remain);
      if (isRunning()) Subprocess::interrupt();
      break;

    } else {
      Timer::sleep(delta);
      remain -= delta;
    }

  remain = kill;
  while (isRunning())
    if (kill && remain < delta) {
      Timer::sleep(remain);
      if (isRunning()) Subprocess::kill();
      break;

    } else {
      Timer::sleep(delta);
      remain -= delta;
    }

  return returnCode;
}


void Subprocess::parse(const string &command, vector<string> &args) {
  bool inSQuote = false;
  bool inDQuote = false;
  bool escape = false;
  unsigned len = command.length();
  string arg;

  for (unsigned i = 0; i < len; i++) {
    if (escape) {
      arg += command[i];
      escape = false;
      continue;
    }

    switch (command[i]) {
    case '\\': escape = true; break;

    case '\'':
      if (inDQuote) arg += command[i];
      else inSQuote = !inSQuote;
      break;

    case '"':
      if (inSQuote) arg += command[i];
      else inDQuote = !inDQuote;
      break;

    case '\t':
    case ' ':
    case '\n':
    case '\r':
      if (inSQuote || inDQuote) arg += command[i];
      else if (arg.length()) {
        args.push_back(arg);
        arg.clear();
      }
      break;

    default: arg += command[i];
    }
  }

  if (arg.length()) args.push_back(arg);
}


string Subprocess::assemble(const vector<string> &args) {
  string command;

  vector<string>::const_iterator it;
  for (it = args.begin(); it != args.end(); it++) {
    const string &arg = *it;

    if (it != args.begin()) command += " ";

    // Check if we need to quote this arg
    bool quote = false;
    for (string::const_iterator it2 = arg.begin(); it2 != arg.end(); it2++)
      if (isspace(*it2)) {quote = true; break;}

    if (quote) command += '"';
    command += arg;
    if (quote) command += '"';
  }

  return command;
}


void Subprocess::closeHandles() {
  closeStdIn();
  closeStdOut();
  closeStdErr();

#ifdef _WIN32
  // Close process and thread handles
  if (p->pi.hProcess) CloseHandle(p->pi.hProcess);
  if (p->pi.hThread) CloseHandle(p->pi.hThread);
  p->pi.hProcess = p->pi.hThread = 0;
#endif

  closePipes();
}


void Subprocess::closePipes() {
  // Close pipe handles
  for (int i = 0; i < 2; i++) {
#ifdef _WIN32
    if (p->stdInPipe[i]) CloseHandle(p->stdInPipe[i]);
    if (p->stdOutPipe[i]) CloseHandle(p->stdOutPipe[i]);
    if (p->stdErrPipe[i]) CloseHandle(p->stdErrPipe[i]);

#else
    if (p->stdInPipe[i]) close(p->stdInPipe[i]);
    if (p->stdOutPipe[i]) close(p->stdOutPipe[i]);
    if (p->stdErrPipe[i]) close(p->stdErrPipe[i]);
#endif

    p->stdInPipe[i] = p->stdOutPipe[i] = p->stdErrPipe[i] = 0;
  }
}
