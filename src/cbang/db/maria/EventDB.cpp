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

#include "EventDB.h"

#include <cbang/event/Base.h>
#include <cbang/event/Event.h>
#include <cbang/event/EventCallback.h>

#include <cbang/util/DefaultCatch.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::MariaDB;


namespace {
  unsigned event_flags_to_db_ready(unsigned flags) {
    unsigned ready = 0;

    if (flags & Event::EventFlag::EVENT_READ) ready |= DB::READY_READ;
    if (flags & Event::EventFlag::EVENT_WRITE) ready |= DB::READY_WRITE;
    if (flags & Event::EventFlag::EVENT_TIMEOUT) ready |= DB::READY_TIMEOUT;

    return ready;
  }


  class CallbackAdapter : public Event::EventCallback {
    EventDB &db;
    SmartPointer<EventDBCallback> cb;

  public:
    CallbackAdapter(EventDB &db, const SmartPointer<EventDBCallback> &cb) :
      db(db), cb(cb){}

    // From cb::Event::EventCallback
    void operator()(Event::Event &e, int fd, unsigned flags) {
      try {
        if (db.continueNB(event_flags_to_db_ready(flags))) {
          try {
            (*cb)(EventDBCallback::EVENTDB_DONE);
          } CATCH_ERROR;

        } else db.addEvent(e);

      } catch (const Exception &e) {
        (*cb)(EventDBCallback::EVENTDB_ERROR);
      }
    }
  };


  class QueryCallback : public Event::EventCallback {
    EventDB &db;
    SmartPointer<EventDBCallback> cb;

    typedef enum {
      QUERY_START,
      QUERY_STORE,
      QUERY_FETCH,
      QUERY_FREE,
      QUERY_NEXT,
      QUERY_DONE,
    } state_t;

    state_t state;

  public:
    QueryCallback(EventDB &db, const SmartPointer<EventDBCallback> &cb) :
      db(db), cb(cb), state(QUERY_START) {}


    bool next() {
      switch (state) {
      case QUERY_START:
        state = QUERY_STORE;
        if (!db.storeResultNB()) return false;

      case QUERY_STORE:
        state = QUERY_FETCH;
        if (!db.fetchRowNB()) return false;

      case QUERY_FETCH:
        while (db.haveRow()) {
          (*cb)(EventDBCallback::EVENTDB_ROW);
          if (!db.fetchRowNB()) return false;
        }
        state = QUERY_FREE;
        if (!db.freeResultNB()) return false;

      case QUERY_FREE:
        if (db.moreResults()) {
          state = QUERY_NEXT;

        } else {
          state = QUERY_DONE;
          (*cb)(EventDBCallback::EVENTDB_DONE);
          return true;
        }

      case QUERY_NEXT:
        state = QUERY_STORE;
        if (!db.nextResultNB()) return false;
        return next();

      default: THROW("Invalid state");
      }
    }


    // From cb::Event::EventCallback
    void operator()(Event::Event &e, int fd, unsigned flags) {
      try {
        if (db.continueNB(event_flags_to_db_ready(flags))) {
          if (!next()) db.renewEvent(e);
        } else db.addEvent(e);

      } catch (const Exception &e) {
        (*cb)(EventDBCallback::EVENTDB_ERROR);
      }
    }
  };
}


EventDB::EventDB(Event::Base &base, st_mysql *db) : DB(db), base(base) {}


unsigned EventDB::getEventFlags() const {
  return
    (waitRead() ? Event::Base::EVENT_READ : 0) |
    (waitWrite() ? Event::Base::EVENT_WRITE : 0) |
    (waitTimeout() ? Event::Base::EVENT_TIMEOUT : 0);
}


void EventDB::newEvent(const SmartPointer<Event::EventCallback> &cb) const {
  assertPending();
  Event::Event &e = base.newEvent(getSocket(), getEventFlags(), cb);
  addEvent(e);
}


void EventDB::renewEvent(Event::Event &e) const {
  assertPending();
  e.renew(getSocket(), getEventFlags());
  addEvent(e);
}


void EventDB::addEvent(Event::Event &e) const {
  if (waitTimeout()) e.add(getTimeout());
  else e.add();
}


void EventDB::callback(const SmartPointer<EventDBCallback> &cb) {
  newEvent(new CallbackAdapter(*this, cb));
}


void EventDB::connect(const SmartPointer<EventDBCallback> &cb,
                      const string &host, const string &user,
                      const string &password, const string &dbName,
                      unsigned port, const string &socketName, flags_t flags) {
  if (connectNB(host, user, password, dbName, port, socketName, flags))
    (*cb)(EventDBCallback::EVENTDB_DONE);
  else callback(cb);
}


void EventDB::query(const SmartPointer<EventDBCallback> &cb, const string &s) {
  SmartPointer<QueryCallback> queryCB = new QueryCallback(*this, cb);
  if (!queryNB(s) || !queryCB->next()) newEvent(queryCB);
}
