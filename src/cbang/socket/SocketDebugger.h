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

#ifndef CBANG_SOCKET_DEBUGGER_H
#define CBANG_SOCKET_DEBUGGER_H

#include "SocketDebugConnection.h"

#include <cbang/SmartPointer.h>

#include <cbang/util/Singleton.h>

#include <cbang/os/Condition.h>

#include <cbang/net/IPAddress.h>

#include <map>
#include <list>

namespace cb {
  class Options;

  namespace Script {
    class Context;
    class Environment;
  }

  class SocketDebugger :  public Singleton<SocketDebugger>, public Condition {
    bool enabled;
    unsigned count;
    unsigned nextID;

    bool capture;
    std::string captureDir;

    typedef std::list<SocketDebugConnection *> connection_list_t;
    typedef std::map<IPAddress, connection_list_t> connection_map_t;

    connection_map_t incoming;
    connection_map_t outgoing;

    ~SocketDebugger();

  public:
    SocketDebugger(Inaccessible) :
      enabled(false), count(0), nextID(0), capture(false),
      captureDir("capture") {}

    bool isEnabled() const {return enabled;}
    void setEnabled(bool x) {enabled = x;}

    bool getCapture() const {return capture;}
    const std::string &getCaptureDirectory() const {return captureDir;}
    uint64_t getNextConnectionID();

    void addOptions(Options &options);
    void addCommands(Script::Environment &env);

    void addIncoming(const IPAddress &addr,
                     SocketDebugConnection * con);
    void addOutgoing(const IPAddress &addr,
                     SocketDebugConnection * con);

    SocketDebugConnection *accept(const IPAddress &addr);
    SocketDebugConnection *connect(const IPAddress &addr);

    // Script commands
    void inject(const Script::Context &ctx);
    void bond(const Script::Context &ctx);
    SocketDebugConnection *makeConnection(const Script::Context &ctx);
  };
}

#endif // CBANG_SOCKET_DEBUGGER_H

