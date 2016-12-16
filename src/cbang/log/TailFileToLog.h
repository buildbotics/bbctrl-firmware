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

#ifndef CBANG_TAIL_FILE_TO_LOG_H
#define CBANG_TAIL_FILE_TO_LOG_H

#include <cbang/SmartPointer.h>
#include <cbang/os/Thread.h>
#include <cbang/log/Logger.h>

#include <string>
#include <iostream>

namespace cb {
  class TailFileToLog : public Thread {
    const std::string filename;
    const std::string prefix;
    const char *logDomain;
    unsigned logLevel;
    SmartPointer<std::iostream> stream;

    static const unsigned bufferSize = 4096;
    char buffer[bufferSize + 1]; // Room for null terminator
    unsigned fill;

  public:
    TailFileToLog(const std::string &filename,
                  const std::string &prefix = std::string(),
                  const char *logDomain = CBANG_LOG_DOMAIN,
                  unsigned logLevel = CBANG_LOG_INFO_LEVEL(1)) :
      filename(filename), prefix(prefix), logDomain(logDomain),
      logLevel(logLevel), fill(0) {}

  protected:
    // From Thread
    void run();

    void log();
  };
}

#endif // CBANG_TAIL_FILE_TO_LOG_H
