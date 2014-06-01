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

#ifndef CB_HTTP_SESSION_MANAGER_H
#define CB_HTTP_SESSION_MANAGER_H

#include "SessionFactory.h"
#include "SessionsTable.h"

#include <cbang/os/Mutex.h>

#include <map>
#include <string>

namespace cb {
  class Options;

  namespace DB {class Database;}

  namespace HTTP {
    class WebContext;

    class SessionManager : public Mutex {
      SmartPointer<SessionFactory> factory;

      typedef std::map<std::string, SessionPtr> sessions_t;
      sessions_t sessions;
      uint64_t sessionLifetime;
      uint64_t sessionTimeout;
      uint64_t lastSessionCleanup;
      std::string sessionCookie;

      SessionsTable table;
      mutable bool dirty;

    public:
      SessionManager(Options &options);

      void setFactory(const SmartPointer<SessionFactory> &factory)
      {this->factory = factory;}

      const std::string &getSessionCookie() const {return sessionCookie;}

      SessionPtr getSessionCookie(WebContext &ctx) const;
      void setSessionCookie(WebContext &ctx) const;

      std::string generateSessionID(WebContext &ctx) const;
      bool hasSession(const std::string &id) const;
      SessionPtr getSession(WebContext &ctx,
                            const std::string &id = std::string()) const;
      SessionPtr findSession(WebContext &ctx,
                             const std::string &id = std::string()) const;
      SessionPtr openSession(WebContext &ctx,
                             const std::string &id = std::string());
      void closeSession(WebContext &ctx, const std::string &id = std::string());

      void create(DB::Database &db);
      void load(DB::Database &db);
      void save(DB::Database &db) const;

      void update();

    protected:
      typedef sessions_t::const_iterator iterator;
      iterator begin() const {return sessions.begin();}
      iterator end() const {return sessions.end();}
    };
  }
}

#endif // CB_HTTP_SESSION_MANAGER_H

