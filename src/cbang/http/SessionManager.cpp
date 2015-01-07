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

#include "SessionManager.h"

#include "Session.h"
#include "Cookie.h"
#include "WebContext.h"

#include <cbang/String.h>
#include <cbang/util/SmartLock.h>
#include <cbang/security/Digest.h>
#include <cbang/config/Options.h>
#include <cbang/time/Time.h>
#include <cbang/db/Database.h>
#include <cbang/db/Transaction.h>
#include <cbang/db/LevelDB.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


SessionManager::SessionManager(Options &options) :
  factory(new SessionFactory), sessionLifetime(Time::SEC_PER_DAY),
  sessionTimeout(Time::SEC_PER_HOUR), lastSessionCleanup(0),
  sessionCookie("sid"), dirty(false) {

  options.pushCategory("Web Server Sessions");

  options.addTarget("session-timeout", sessionTimeout, "The max maximum "
                    "amount of time in seconds since the last time a session "
                    "was used before it timesout.  Zero for no session "
                    "timeout.");
  options.addTarget("session-lifetime", sessionLifetime, "The maximum "
                    "session lifetime in seconds.  Zero for unlimited "
                    "session lifetime.");
  options.addTarget("session-cookie", sessionCookie, "The name of the "
                    "session cookie.");

  options.popCategory();
}


SessionPtr SessionManager::getSessionCookie(WebContext &ctx) const {
  if (!ctx.getRequest().hasCookie(sessionCookie)) return 0;
  return getSession(ctx, ctx.getRequest().getCookie(sessionCookie));
}


void SessionManager::setSessionCookie(WebContext &ctx) const {
  SessionPtr session = ctx.getSession();

  Cookie cookie(sessionCookie, session->getID(), "", "",
                session->getCreationTime() + sessionLifetime,
                sessionLifetime, true, true);

  ctx.getResponse().set("Set-Cookie", cookie.toString());
}


string SessionManager::generateSessionID(WebContext &ctx) const {
  Digest digest("md5");

  if (ctx.getRequest().has("User-Agent"))
    digest.update(ctx.getRequest().get("User-Agent"));
  digest.updateWith(Time::now());

  return digest.toHexString();
}


bool SessionManager::hasSession(const string &id) const {
  SmartLock lock(this);
  return sessions.find(id) != sessions.end();
}


SessionPtr SessionManager::getSession(WebContext &ctx,
                                      const std::string &_id) const {
  string id = _id;
  if (id.empty()) id = ctx.getRequest().findCookie(sessionCookie);
  if (id.empty()) THROW("Session ID is not set");

  if (!ctx.getSession().isNull() && id == ctx.getSession()->getID())
    return ctx.getSession();

  SmartLock lock(this);

  iterator it = sessions.find(id);
  if (it == end()) THROWS("Session ID '" << id << "' does not exist");
  SmartPointer<Session> session = it->second;

  // Check that IP address matches
  if (ctx.getClientIP().getIP() != session->getIP().getIP())
    THROWS("Session ID (" << id << ") IP address changed from "
           << ctx.getClientIP() << " to " << session->getIP());

  session->touch(); // Update timestamp
  ctx.setSession(session);
  dirty = true;

  return session;
}


SessionPtr SessionManager::findSession(WebContext &ctx,
                                       const std::string &id) const {
  try {
    return getSession(ctx, id);
  } catch (const Exception &e) {
    LOG_DEBUG(5, "Session not found: " << e);
    return 0;
  }
}


SessionPtr SessionManager::openSession(WebContext &ctx, const string &_id) {
  string id = _id;
  if (id.empty()) id = generateSessionID(ctx);

  SmartLock lock(this);

  // Get the session
  SessionPtr session;
  sessions_t::iterator it = sessions.find(id);
  if (it != sessions.end()) session = it->second;

  if (session.isNull()) {
    session = factory->createSession(id);
    sessions.insert(sessions_t::value_type(id, session));
  }

  // Set IP
  IPAddress ip = ctx.getClientIP();
  ip.setPort(0);
  session->setIP(ip);

  session->touch(); // Update timestamp
  ctx.setSession(session);
  dirty = true;

  return session;
}


void SessionManager::closeSession(WebContext &ctx, const string &_id) {
  string id = _id;
  if (id.empty()) id = ctx.getRequest().findCookie(sessionCookie);
  if (id.empty()) return;

  SmartLock lock(this);
  sessions.erase(id);
  dirty = true;
}


void SessionManager::create(DB::Database &db) {
  table.create(db);
}


void SessionManager::load(DB::Database &db) {
  SmartPointer<DB::Statement> readStmt = table.makeReadStmt(db);
  sessions_t sessions;

  // Load sessions
  while (readStmt->next()) {
    SmartPointer<Session> session = factory->createSession("");
    table.readRow(readStmt, *session);

    sessions.insert(sessions_t::value_type(session->getID(), session));
  }

  // Replace current
  SmartLock lock(this);
  this->sessions = sessions;

  // Update
  update();
}


void SessionManager::save(DB::Database &db) const {
  if (!dirty) return;
  dirty = false;

  SmartPointer<DB::Statement> writeStmt = table.makeWriteStmt(db);
  SmartPointer<DB::Transaction> transaction = db.begin();

  // Delete old sessions
  table.deleteAll(db);

  // Write sessions
  SmartLock lock(this);
  for (iterator it = begin(); it != end(); it++) {
    table.bindWriteStmt(writeStmt, *it->second);
    writeStmt->execute();
  }

  db.commit();
}


#ifdef HAVE_LEVELDB
void SessionManager::load(LevelDB db) {
  sessions_t sessions;

  // Load sessions
  LevelDB nsDB = db.ns("session:");

  for (LevelDB::Iterator it = nsDB.begin(); it.valid(); it++) {
    SmartPointer<Session> session = factory->createSession(it.key());
    session->parse(it.value());
    sessions.insert(sessions_t::value_type(session->getID(), session));
  }

  // Replace current
  SmartLock lock(this);
  this->sessions = sessions;

  // Update
  update();
}


void SessionManager::save(LevelDB db) const {
  if (!dirty) return;
  dirty = false;

  LevelDB nsDB = db.ns("session:");
  LevelDB::Batch batch = nsDB.batch();

  // Delete old sessions
  for (LevelDB::Iterator it = nsDB.begin(); it.valid(); it++)
    batch.erase(it.key());

  // Write sessions
  SmartLock lock(this);
  for (iterator it = begin(); it != end(); it++)
    nsDB.set(it->second->getID(), SSTR(*it->second));
}
#endif // HAVE_LEVELDB


void SessionManager::update() {
  // Cleanup old sessions
  if (sessionLifetime || sessionTimeout) {
    SmartLock lock(this);

    uint64_t now = Time::now();
    if (lastSessionCleanup + 5 < now) {
      lastSessionCleanup = now;

      vector<string> remove;
      for (iterator it = begin(); it != end(); it++)
        if (it->second->getLastUsed() + sessionTimeout < now ||
            it->second->getCreationTime() + sessionLifetime < now)
          remove.push_back(it->first);

      for (unsigned i = 0; i < remove.size(); i++)
        sessions.erase(remove[i]);
    }
  }

  dirty = false;
}
