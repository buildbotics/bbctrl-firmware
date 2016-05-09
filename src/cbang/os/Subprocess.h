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

#ifndef CBANG_SUBPROCESS_H
#define CBANG_SUBPROCESS_H

#include <cbang/util/StringMap.h>

#include <cbang/enum/ProcessPriority.h>

#include <string>
#include <iostream>
#include <vector>

namespace cb {
  class Subprocess : public StringMap {
  public:
    enum {
      SHELL                   = 1 << 0,
      REDIR_STDIN             = 1 << 1,
      REDIR_STDOUT            = 1 << 2,
      REDIR_STDERR            = 1 << 3,
      MERGE_STDOUT_AND_STDERR = 1 << 4,
      CLEAR_ENVIRONMENT       = 1 << 5,
      NULL_STDOUT             = 1 << 6,
      NULL_STDERR             = 1 << 7,
      CREATE_PROCESS_GROUP    = 1 << 8,
      W32_HIDE_WINDOW         = 1 << 9,
      W32_WAIT_FOR_INPUT_IDLE = 1 << 10,
      };

  protected:
    struct Private;
    Private *p;

    bool running;
    bool wasKilled;
    bool dumpedCore;
    bool signalGroup;
    int returnCode;

    std::string wd;

  public:
    Subprocess();
    ~Subprocess();

    bool isRunning();
    uint64_t getPID() const;
    int getReturnCode() const {return returnCode;}
    bool getWasKilled() const {return wasKilled;}
    bool getDumpedCore() const {return dumpedCore;}

#ifdef _WIN32
    typedef void *handle_t;
#else
    typedef int handle_t;
#endif

    unsigned createPipe(bool toChild);
    handle_t getPipeHandle(unsigned i, bool childEnd = true);

    const SmartPointer<std::iostream> &getStream(unsigned i) const;
    std::ostream &getStdIn() const {return *getStream(0);}
    std::istream &getStdOut() const {return *getStream(1);}
    std::istream &getStdErr() const {return *getStream(2);}

    void closeStream(unsigned i);
    void closeStdIn() {closeStream(0);}
    void closeStdOut() {closeStream(1);}
    void closeStdErr() {closeStream(2);}

    void setWorkingDirectory(const std::string &wd) {this->wd = wd;}

    void exec(const std::vector<std::string> &args, unsigned flags = 0,
              ProcessPriority priority = ProcessPriority::PRIORITY_INHERIT);
    void exec(const std::string &command, unsigned flags = 0,
              ProcessPriority priority = ProcessPriority::PRIORITY_INHERIT);

    bool kill(bool nonblocking = false);
    void interrupt();
    int wait(bool nonblocking = false);
    int waitFor(double interrupt = 0, double kill = 0);

    static void parse(const std::string &command,
                      std::vector<std::string> &args);
    static std::string assemble(const std::vector<std::string> &args);

  protected:
    void closeHandles();
    void closePipes();
  };
}

#endif // CBANG_SUBPROCESS_H
