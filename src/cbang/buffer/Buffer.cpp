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

#include "Buffer.h"

using namespace cb;
using namespace std;


static const unsigned BUFFER_SIZE = 4096;


unsigned Buffer::writeTo(ostream &stream) {
  char buf[BUFFER_SIZE];
  streamsize total = 0;

  while (!isEmpty() && stream) {
    streamsize size = read(buf, BUFFER_SIZE);
    if (size) {
      stream.write(buf, size);
      if (stream) total += size;
      else break;
    }
  }

  return total;
}


unsigned Buffer::readFrom(istream &stream) {
  char buf[BUFFER_SIZE];
  streamsize total = 0;

  while (!isFull() && stream) {
    stream.read(buf, BUFFER_SIZE);
    streamsize size = stream.gcount();

    if (size) {
      write(buf, size);
      total += size;
    }
  }

  return total;
}
