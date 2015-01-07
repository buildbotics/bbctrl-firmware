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

#ifndef CBANG_SOCKET_SET_H
#define CBANG_SOCKET_SET_H

#include <map>

namespace cb {
  class Socket;

  class SocketSet {
    struct private_t;
    private_t *p;
    int maxFD;

  public:
    typedef enum {
      READ   = 1,
      WRITE  = 2,
      EXCEPT = 4,
      ALL    = READ | WRITE | EXCEPT
    } type_t;

    SocketSet();
    SocketSet(const SocketSet &s);
    ~SocketSet();

    void clear();
    void add(const Socket &socket, int type = ALL);
    void remove(const Socket &socket, int type = ALL);
    bool isSet(const Socket &socket, int type = ALL) const;

    /**
     * Check if the sockets in the set are ready.
     *
     * @param timeout If < 0 block indefinitely, if == 0 don't block,
     *   if > 0 block for at most @param timeout seconds.
     *
     * @return True if at least one socket in the set is ready.
     */
    bool select(double timeout = -1);
  };
}

#endif // CBANG_SOCKET_SET_H

