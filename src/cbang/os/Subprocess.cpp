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


namespace {
#ifdef _WIN32
  HANDLE OpenNUL() {
    // Setup security attributes for pipe inheritance
    SECURITY_ATTRIBUTES sAttrs;
    memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
    sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
    sAttrs.bInheritHandle = TRUE;
    sAttrs.lpSecurityDescriptor = 0;

    return CreateFile("NUL", GENERIC_WRITE, 0, &sAttrs, OPEN_EXISTING, 0, 0);
  }
#endif


  struct Pipe {
    bool toChild;

#ifdef _WIN32
    HANDLE handles[2];
#else
    int handles[2];
#endif

    SmartPointer<iostream> stream;


    explicit Pipe(bool toChild) : toChild(toChild) {
      handles[0] = handles[1] = 0;
    }


    void create() {
#ifdef _WIN32
      // Setup security attributes for pipe inheritance
      SECURITY_ATTRIBUTES sAttrs;
      memset(&sAttrs, 0, sizeof(SECURITY_ATTRIBUTES));
      sAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
      sAttrs.bInheritHandle = TRUE;
      sAttrs.lpSecurityDescriptor = 0;

      if (!CreatePipe(&handles[0], &handles[1], &sAttrs, 0))
        THROWS("Failed to create pipe: " << SysError());

      // Don't inherit other handle
      if (!SetHandleInformation(handles[toChild ? 1 : 0],
                                HANDLE_FLAG_INHERIT, 0))
        THROWS("Failed to clear pipe inherit flag: " << SysError());

#else
      if (pipe(handles)) THROWS("Failed to create pipe: " << SysError());
#endif
    }


#ifndef _WIN32
    void inChildProc(int target = -1) {
      // Close parent end
      ::close(handles[toChild ? 1 : 0]);

      // Move to target
      if (target != -1 && dup2(handles[toChild ? 0 : 1], target) != target)
        perror("Moving file descriptor");
    }
#endif


    void close() {
      for (unsigned i = 0; i < 2; i++)
        if (handles[i]) {
#ifdef _WIN32
          CloseHandle(handles[i]);
#else
          ::close(handles[i]);
#endif
          handles[i] = 0;
        }
    }


    void openStream() {
      typedef io::stream<io::file_descriptor> stream_t;

      if (toChild) {
        stream = new stream_t(handles[1], BOOST_CLOSE_HANDLE);
        handles[1] = 0; // Don't close this end later in close()

      } else {
        stream = new stream_t(handles[0], BOOST_CLOSE_HANDLE);
        handles[0] = 0; // Don't close this end later in close()
      }
    }


    void closeStream() {stream.release();}
  };
}


struct Subprocess::Private {
#ifdef _WIN32
  PROCESS_INFORMATION pi;
  STARTUPINFO si;
#else
  pid_t pid;
#endif

  vector<Pipe> pipes;

  Private() {
#ifdef _WIN32
    memset(&pi, 0, sizeof(PROCESS_INFORMATION));
    memset(&si, 0, sizeof(STARTUPINFO));
#else
    pid = 0;
#endif

    pipes.push_back(Pipe(true)); // stdin
    pipes.push_back(Pipe(false)); // stdout
    pipes.push_back(Pipe(false)); // stderr
 }
};


Subprocess::Subprocess() :
  p(new Private), running(false), wasKilled(false), dumpedCore(false),
  signalGroup(false),
  returnCode(0) {
}


Subprocess::~Subprocess() {
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


unsigned Subprocess::createPipe(bool toChild) {
  p->pipes.push_back(Pipe(toChild));
  p->pipes.back().create();
  return p->pipes.size() - 1;
}


Subprocess::handle_t Subprocess::getPipeHandle(unsigned i, bool childEnd) {
  if (p->pipes.size() <= i) THROWS("Subprocess does not have pipe " << i);
  return p->pipes[i].handles[(p->pipes[i].toChild ^ childEnd) ? 1 : 0];
}


const SmartPointer<iostream> &Subprocess::getStream(unsigned i) const {
  if (p->pipes.size() <= i || p->pipes[i].stream.isNull())
    THROWS("Subprocess stream " << i << " not available");
  return p->pipes[i].stream;
}


void Subprocess::closeStream(unsigned i) {
  if (p->pipes.size() <= i) THROWS("Subprocess does not have pipe " << i);
  p->pipes[i].closeStream();
}


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
  //closeHandles();

  try {
    // Create pipes
    if (flags & REDIR_STDIN) p->pipes[0].create();
    if (flags & REDIR_STDOUT) p->pipes[1].create();
    if (flags & REDIR_STDERR) p->pipes[2].create();

#ifdef _WIN32
    // Clear PROCESS_INFORMATION
    memset(&p->pi, 0, sizeof(PROCESS_INFORMATION));

    // Configure STARTUPINFO
    memset(&p->si, 0, sizeof(STARTUPINFO));
    p->si.cb = sizeof(STARTUPINFO);
    p->si.hStdInput =
      (flags & REDIR_STDIN) ? p->pipes[0].handles[0] :
      GetStdHandle(STD_INPUT_HANDLE);
    p->si.hStdOutput =
      (flags & REDIR_STDOUT) ? p->pipes[1].handles[1] :
      GetStdHandle(STD_OUTPUT_HANDLE);
    p->si.hStdError = (flags & REDIR_STDERR) ? p->pipes[2].handles[1] :
      ((flags & MERGE_STDOUT_AND_STDERR) ? p->pipes[1].handles[1] :
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
      if (!SystemUtilities::isAbsolute(wd)) wd = SystemUtilities::absolute(wd);
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

    if (flags & W32_WAIT_FOR_INPUT_IDLE) {
      int ret = WaitForInputIdle(p->pi.hProcess, 5000);
      if (ret == (int)WAIT_TIMEOUT) THROW("Wait timedout");
      if (ret == (int)WAIT_FAILED) THROW("Wait failed");
    }

#else // _WIN32
    // Convert args
    vector<char *> args;
    for (unsigned i = 0; i < _args.size(); i++)
      args.push_back((char *)_args[i].c_str());
    args.push_back(0); // Sentinal

    p->pid = fork();

    if (!p->pid) { // Child
      // Process group
      if (flags & CREATE_PROCESS_GROUP) setpgid(0, 0);

      // Configure pipes
      if (flags & REDIR_STDIN) p->pipes[0].inChildProc(0);
      if (flags & REDIR_STDOUT) p->pipes[1].inChildProc(1);

      if (flags & MERGE_STDOUT_AND_STDERR)
        if (dup2(1, 2) != 2) perror("Copying stdout to stderr");

      if (flags & REDIR_STDERR) p->pipes[2].inChildProc(2);

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

      for (unsigned i = 3; i < p->pipes.size(); i++)
        p->pipes[i].inChildProc();

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
  if (flags & REDIR_STDIN) p->pipes[0].openStream();
  if (flags & REDIR_STDOUT) p->pipes[1].openStream();
  if (flags & REDIR_STDERR) p->pipes[2].openStream();
  for (unsigned i = 3; i < p->pipes.size(); i++)
    p->pipes[i].openStream();

  // Close pipe child ends
  closePipes();

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
  if (!(signalGroup ? ::killpg : ::kill)((pid_t)getPID(), SIGINT)) return;
#endif // _WIN32

  THROWS("Failed to interrupt process " << getPID() << ": " << SysError());
}


bool Subprocess::kill(bool nonblocking) {
  if (!running) THROW("Process not running!");

#ifdef _WIN32
  HANDLE h = p->pi.hProcess;
  if (!h) h = OpenProcess(PROCESS_TERMINATE, false, getPID());
  if (!h || !TerminateProcess(h, -1)) return false;

#else
  if (!(signalGroup ? ::killpg : ::kill)((pid_t)getPID(), SIGKILL))
    return false;
#endif

  if (!nonblocking) wait();

  wasKilled = true;
  return true;
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
#ifdef _WIN32
  // Close process & thread handles
  if (p->pi.hProcess) {CloseHandle(p->pi.hProcess); p->pi.hProcess = 0;}
  if (p->pi.hThread) {CloseHandle(p->pi.hThread); p->pi.hThread = 0;}
#endif

  for (unsigned i = 0; i < p->pipes.size(); i++)
    p->pipes[i].closeStream();

  closePipes();
}


void Subprocess::closePipes() {
  for (unsigned i = 0; i < p->pipes.size(); i++)
    p->pipes[i].close();
}
