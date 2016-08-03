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

#include "Processor.h"

#include "Function.h"
#include "MemberFunctor.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>
#include <cbang/socket/Socket.h>
#include <cbang/socket/SocketDevice.h>

#include <istream>
#include <ctype.h>
#include <string.h>

using namespace std;
using namespace cb;
using namespace cb::Script;


Processor::Processor(const string &name) : Environment(name) {
  typedef Processor P;
  typedef MemberFunctor<P> MF;

  add(new MF("exit", this, &P::exit, 0, 0, "Exit the command processor"));
  add(new MF("quit", this, &P::exit, 0, 0, "Exit the command processor"));
}


void Processor::run(Handler &handler, Socket &socket) {
  parent = &handler;

  socket.setBlocking(true);
  socket.setSendTimeout(30);
  socket.setReceiveTimeout(0.25);
  socket.setKeepAlive(true);

  SocketStream out(socket);
  Context ctx(*this, out);

  const unsigned size = 4096;
  unsigned fill = 0;
  char buffer[size];

  Handler::eval(ctx, "$(eval $greeting $prompt)");
  out << flush;

  quit = false;
  while (!quit && !out.fail() && socket.isOpen()) {
    update(Context(*this, out));

    // Read
    streamsize bytes = socket.read(buffer + fill, size - fill);
    if (bytes < 0) return;
    if (!bytes) continue;
    fill += bytes;

    // Parse lines
    while (fill) {
      string line;
      unsigned i;

      for (i = 0; i < fill; i++)
        if (buffer[i] == '\n' || buffer[i] == '\r') {
          line = string(buffer, i);

          // Remove line
          i++;
          fill -= i;
          memmove(buffer, buffer + i, fill);
          break;
        }
      if (i == fill && line.empty()) break;

      // Process line
      try {
        Arguments args;
        Arguments::parse(args, line);
        if (!args.size()) continue;

        bool handled = eval(Context(*this, out, args));
        if (!handled)
          out << "ERROR: unknown command or variable '" << args[0] << "'\n";

      } catch (const Exception &e) {
        out << "ERROR: " << e << '\n';
      }

      if (quit) break;

      Handler::eval(ctx, "$(eval $prompt)");
      out << flush;
    }
  }

  parent = 0;
}


void Processor::exit(const Context &ctx) {
  quit = true;
}
