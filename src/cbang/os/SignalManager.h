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

#ifndef CBANG_SIGNAL_MANAGER_H
#define CBANG_SIGNAL_MANAGER_H

#include <map>

#include "SignalHandler.h"
#include "Thread.h"
#include "Condition.h"

#include <cbang/util/Singleton.h>

#include <signal.h>

#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#ifndef SIGHUP
#define SIGHUP 1
#endif

namespace cb {
  class SignalManager :
    public Singleton<SignalManager>, protected Thread, public Condition {

    struct private_t;
    private_t *pri;

    bool enabled;

    typedef std::map<int, SignalHandler *> handlers_t;
    handlers_t handlers;

    ~SignalManager();

  public:
    SignalManager(Inaccessible);

    void setEnabled(bool x);
    bool isEnabled() const {return enabled;}

    void addHandler(int signal, SignalHandler *handler);
    void ignoreSignal(int signal) {addHandler(signal, 0);}
    void removeHandler(int signal);

    void signal(int sig);

  protected:
    // From Thread
    void run();

    void update();
    void restore();

    void block(int sig);
    void unblock(int sig);

  public:
    static const char *signalString(int sig);
  };
}

#endif // CBANG_SIGNAL_MANAGER_H

