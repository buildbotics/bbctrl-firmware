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

#include "Transfer.h"

using namespace cb;
using namespace std;

#define BUFFER_SIZE ((streamsize)64 * 1024)


streamsize cb::transfer(istream &in, ostream &out, streamsize length,
                        SmartPointer<TransferCallback> callback) {
  char buffer[BUFFER_SIZE];
  streamsize total = 0;

  while (!in.fail() && !out.fail()) {
    in.read(buffer, length ? min(length, BUFFER_SIZE) : BUFFER_SIZE);
    streamsize bytes = in.gcount();
    out.write(buffer, bytes);

    total += bytes;
    if (!callback.isNull() && !callback->transferCallback(bytes)) break;

    if (length) {
      length -= bytes;
      if (!length) break;
    }
  }

  out.flush();

  if (out.fail() || length) THROW("Transfer failed");

  return total;
}
