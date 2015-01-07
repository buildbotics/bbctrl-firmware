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

#include "SocketDebugger.h"

#include <cbang/config/Options.h>

#include <cbang/net/IPAddress.h>

#include <cbang/os/SystemUtilities.h>
#include <cbang/os/ExitSignalHandler.h>

#include <cbang/util/SmartLock.h>
#include <cbang/log/Logger.h>
#include <cbang/time/Timer.h>

#include <cbang/script/MemberFunctor.h>
#include <cbang/script/Environment.h>
#include <cbang/script/Context.h>
#include <cbang/script/Arguments.h>

using namespace std;
using namespace cb;
using namespace cb::Script;

SINGLETON_DECL(SocketDebugger);


SocketDebugger::~SocketDebugger() {
  connection_map_t::iterator it;
  connection_list_t::iterator it2;

  for (it = incoming.begin(); it != incoming.end(); it++)
    for (it2 = it->second.begin(); it2 != it->second.end(); it2++) {
      if (!(*it2)->permanent)
        LOG_ERROR("Incoming debug connection from " << it->first
                  << " was not used");
      delete *it2;
    }

  for (it = outgoing.begin(); it != outgoing.end(); it++)
    for (it2 = it->second.begin(); it2 != it->second.end(); it2++) {
      if (!(*it2)->permanent)
        LOG_ERROR("Outgoing debug connection to " << it->first
                  << " was not used");
      delete *it2;
    }
}


uint64_t SocketDebugger::getNextConnectionID() {
  SmartLock lock(this);
  return nextID++;
}


void SocketDebugger::addOptions(Options &options) {
  options.pushCategory("Debugging");
  options.addTarget("debug-sockets", enabled,
                    "Enable the socket debugger.  Normal socket operation will "
                    "not be available.");
  options.addTarget("capture-sockets", capture, "Capture incoming and outgoing "
                    "socket data and save to 'capture-directory'.");
  options.addTarget("capture-directory", captureDir, "Directory to store "
                    "captured network data.");
  options.popCategory();
}


void SocketDebugger::addCommands(Script::Environment &env) {
  env.add(new Script::MemberFunctor<SocketDebugger>
          ("inject", this, &SocketDebugger::inject, 2, 4, "Inject a packet "
           "file to a listening debug socket.  Will wait until packet is "
           "processed.",
           "<ip>:<port> <input> [output] [ip:port]"));
  env.add(new Script::MemberFunctor<SocketDebugger>
          ("bond", this, &SocketDebugger::bond, 2, 4, "Bond a packet file to "
           "a outgoing debug socket connection.",
           "<ip>:<port> <input> [output] [ip:port]"));
}


void SocketDebugger::addIncoming(const IPAddress &addr,
                                 SocketDebugConnection *con) {
  LOG_DEBUG(5, "SocketDebugger::addIncoming(" << addr << ")");

  SmartLock lock(this);
  incoming[addr].push_back(con);
  count++;

  broadcast();
}


void SocketDebugger::addOutgoing(const IPAddress &addr,
                                 SocketDebugConnection *con) {
  LOG_DEBUG(5, "SocketDebugger::addOutgoing(" << addr << ")");

  SmartLock lock(this);
  outgoing[addr].push_back(con);
  if (!con->permanent) count++;
}


SocketDebugConnection *SocketDebugger::accept(const IPAddress &addr) {
  LOG_DEBUG(5, "SocketDebugger::accept(" << addr << ")");

  SmartLock lock(this);

  connection_map_t::iterator it;

  for (int i = 0; i < 2; i++) {
    it = incoming.find(addr);

    if (it == incoming.end() || it->second.empty()) {
      if (i) return 0; // Only retry once

      // If condition triggered try again
      if (timedWait(0.1)) continue;
      return 0;
    }

    break; // We found something
  }

  SocketDebugConnection *con = it->second.front();
  con->accept(addr);
  it->second.pop_front();
  count--;

  return con;
}


SocketDebugConnection *SocketDebugger::connect(const IPAddress &addr) {
  LOG_DEBUG(5, "SocketDebugger::connect(" << addr << ")");

  SmartLock lock(this);

  connection_map_t::iterator it = outgoing.find(addr);
  if (it == outgoing.end() || it->second.empty())
    THROWS("SocketDebugger not ready for outgoing connection to " << addr);

  SocketDebugConnection *con = it->second.front();
  con->connect(addr);

  if (!con->permanent) {
    it->second.pop_front();
    count--;
  }

  return con;
}


void SocketDebugger::inject(const Context &ctx) {
  if (!enabled) THROW("SocketDebugger not enabled");

  IPAddress addr(ctx.args[1]);
  SocketDebugConnection *con = makeConnection(ctx);
  addIncoming(addr, con);

  // Wait
  {
    SmartLock lock(con);
    while (!con->isDone()) {
      if (ExitSignalHandler::instance().getSignalCount()) return;
      con->timedWait(0.1);
    }
  }

  delete con;
}


void SocketDebugger::bond(const Context &ctx) {
  if (!enabled) THROW("SocketDebugger not enabled");

  IPAddress addr(ctx.args[1]);
  SocketDebugConnection *con = makeConnection(ctx);
  addOutgoing(addr, con);
}


SocketDebugConnection *SocketDebugger::makeConnection(const Context &ctx) {
  IPAddress from;
  if (ctx.args.size() > 4) from = IPAddress(ctx.args[4]);

  // Open streams
  SmartPointer<iostream> in = SystemUtilities::open(ctx.args[2], ios::in);
  SmartPointer<iostream> out;
  if (ctx.args.size() > 3)
    out = SystemUtilities::open(ctx.args[3], ios::out | ios::trunc);

  return new SocketDebugConnection(from, in, out);
}
