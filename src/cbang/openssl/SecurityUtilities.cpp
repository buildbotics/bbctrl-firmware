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

#include "SecurityUtilities.h"

#include <cbang/util/DefaultCatch.h>
#include <cbang/os/SysError.h>

#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else // _WIN32
#include <termios.h>
#endif // _WIN32

#include <stdio.h>

using namespace cb;
using namespace std;


namespace cb {
  namespace SecurityUtilities {

    string getpass(const string &msg) {
#ifdef _WIN32
      HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);

      DWORD mode;
      if (!GetConsoleMode(handle, &mode))
        THROWS("Failed to get console mode: " << SysError());

      DWORD newMode = mode | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT;
      newMode &= ~ENABLE_ECHO_INPUT;

      if (!SetConsoleMode(handle, newMode))
        THROWS("Failed to turn off echo for password entry: " << SysError());

#else
      struct termios oflags, nflags;

      // Disable echo
      tcgetattr(fileno(stdin), &oflags);
      nflags = oflags;
      nflags.c_lflag &= ~ECHO;
      nflags.c_lflag |= ECHONL;

      if (tcsetattr(fileno(stdin), TCSANOW, &nflags) != 0)
        THROWS("Failed to turn off echo for password entry: " << SysError());
#endif

      string pass;
      try {
        while (pass.empty()) {
          char buffer[1024];

          cout << msg << flush;
          cin.getline(buffer, 1024);
          pass = buffer;
        }
      } CBANG_CATCH_ERROR;

#ifdef _WIN32
      if (!SetConsoleMode(handle, mode))
        THROWS("Failed to restore echo after password entry: " << SysError());

#else
      // Restore terminal
      if (tcsetattr(fileno(stdin), TCSANOW, &oflags) != 0)
        THROWS("Failed to restore echo after password entry: " << SysError());
#endif

      return pass;
    }

  } // namespace SecurityUtilities
}  // namespace cb
