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

#ifdef _MSC_VER

#include "Win32EventLog.h"

#include <cbang/Exception.h>

#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

using namespace cb;
using namespace std;


Win32EventLog::Win32EventLog(const std::string &source, const string &server) :
  source(source),
  handle(RegisterEventSource(server.empty() ? 0 : server.c_str(),
                             source.c_str())) {
  if (!handle) THROW("Failed to register WIN32 event source");
}


Win32EventLog::~Win32EventLog() {
  if (handle) DeregisterEventSource(handle);
}


void Win32EventLog::log(string &message, unsigned type, unsigned category,
                        unsigned id) const {
  LPCSTR strs[2] = {(LPCSTR)source.c_str(), (LPCSTR)message.c_str()};
  ReportEvent(handle, (WORD)type, (WORD)category, (DWORD)id, 0, 2, 0, strs, 0);
}

#endif // _MSC_VER
