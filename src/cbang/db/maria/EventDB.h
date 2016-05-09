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

#ifndef CB_MARIADB_EVENT_DB_H
#define CB_MARIADB_EVENT_DB_H

#include "DB.h"
#include "EventDBCallback.h"


namespace cb {
  namespace Event {
    class Event;
    class EventCallback;
    class Base;
  }

  namespace JSON {class Value;}

  namespace MariaDB {
    class EventDB : public DB {
      Event::Base &base;

    public:
      EventDB(Event::Base &base, st_mysql *db = 0);

      unsigned getEventFlags() const;

      void newEvent(const SmartPointer<Event::EventCallback> &cb) const;
      void renewEvent(Event::Event &e) const;
      void addEvent(Event::Event &e) const;

      void callback(const SmartPointer<EventDBCallback> &cb);
      void connect(const SmartPointer<EventDBCallback> &cb,
                   const std::string &host = "localhost",
                   const std::string &user = "root",
                   const std::string &password = std::string(),
                   const std::string &dbName = std::string(),
                   unsigned port = 3306,
                   const std::string &socketName = std::string(),
                   flags_t flags = FLAG_NONE);
      void query(const SmartPointer<EventDBCallback> &cb, const std::string &s,
                 const SmartPointer<JSON::Value> &dict = 0);


      template <class T>
      void callback(T *obj,
                    typename EventDBMemberFunctor<T>::member_t member) {
        callback(new EventDBMemberFunctor<T>(obj, member));
      }


      template <class T>
      void connect(T *obj,
                   typename EventDBMemberFunctor<T>::member_t member,
                   const std::string &host = "localhost",
                   const std::string &user = "root",
                   const std::string &password = std::string(),
                   const std::string &dbName = std::string(),
                   unsigned port = 3306,
                   const std::string &socketName = std::string(),
                   flags_t flags = FLAG_NONE) {
        connect(new EventDBMemberFunctor<T>(obj, member), host, user, password,
                dbName, port, socketName, flags);
      }


      template <class T>
      void query(T *obj, typename EventDBMemberFunctor<T>::member_t member,
                 const std::string &s,
                 const SmartPointer<JSON::Value> &dict = 0) {
        query(new EventDBMemberFunctor<T>(obj, member), s, dict);
      }


      // From DB
      using DB::connect;
      using DB::query;
    };
  }
}

#endif // CB_MARIADB_EVENT_DB_H
