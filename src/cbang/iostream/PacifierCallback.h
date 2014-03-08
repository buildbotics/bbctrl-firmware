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

#ifndef CBANG_PASSIFIER_CALLBACK_H
#define CBANG_PASSIFIER_CALLBACK_H

#include "Transfer.h"

#include <string>

namespace cb {
  class Application;

  // TODO This should be called SocketPacifier and moved to ../socket
  class PacifierCallback : public TransferCallback {
    Application &app;
    const std::string prefix;
    unsigned rate;
    unsigned level;
    std::streamsize completed;
    std::streamsize total;
    uint64_t last;

  public:
    PacifierCallback(Application &app, std::streamsize total,
                     const std::string &prefix, unsigned rate = 5,
                     unsigned level = 1);

    // From TransferCallback
    bool transferCallback(std::streamsize count);
  };
}

#endif // CBANG_PASSIFIER_CALLBACK_H
