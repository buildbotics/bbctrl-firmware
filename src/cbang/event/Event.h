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

#ifndef CB_EVENT_EVENT_H
#define CB_EVENT_EVENT_H

#include "Base.h"

#include <cbang/SmartPointer.h>

struct event;


namespace cb {
  namespace Event {
    class EventCallback;

    class Event : public EventFlag {
      event *e;
      SmartPointer<EventCallback> cb;
      bool selfDestruct;

    public:
      Event(Base &base, int fd, unsigned events,
            const SmartPointer<EventCallback> &cb, bool selfDestruct = false);
      Event(Base &base, int signal, const SmartPointer<EventCallback> &cb,
            bool selfDestruct = false);
      Event(event *e, const SmartPointer<EventCallback> &cb,
            bool selfDestruct = false) :
        e(e), cb(cb), selfDestruct(selfDestruct) {}
      virtual ~Event();

      event *getEvent() const {return e;}

      bool isPending(unsigned events = ~0) const;

      void assign(Base &base, int fd, unsigned events,
                  const SmartPointer<EventCallback> &cb);
      void renew(int fd, unsigned events);
      void renew(int signal);
      void add(double t);
      void add();
      void del();

      void call(int fd, short flags);
    };
  }
}

#endif // CB_EVENT_EVENT_H

