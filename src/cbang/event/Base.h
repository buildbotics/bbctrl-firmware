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

#ifndef CB_EVENT_BASE_H
#define CB_EVENT_BASE_H

#include "EventCallback.h"

#include <cbang/SmartPointer.h>

struct event_base;


namespace cb {
  namespace Event {
    class Event;
    class EventCallback;

    class Base {
      event_base *base;

    public:
      typedef enum {
        EVENT_NONE     = 0,
        EVENT_TIMEOUT  = 1 << 0,
        EVENT_READ     = 1 << 1,
        EVENT_WRITE    = 1 << 2,
        EVENT_SIGNAL   = 1 << 3,
        EVENT_PERSIST  = 1 << 4,
        EVENT_ET       = 1 << 5,
        EVENT_FINALIZE = 1 << 6,
        EVENT_CLOSED   = 1 << 7,
      } event_t;

      Base();
      ~Base();

      struct event_base *getBase() const {return base;}

      void assign(Event &event, int fd, event_t events,
                  const SmartPointer<EventCallback> &cb);

      SmartPointer<Event> newEvent(int fd, unsigned events,
                                   const SmartPointer<EventCallback> &cb);
      SmartPointer<Event> newSignal(int signal,
                                    const SmartPointer<EventCallback> &cb);

      template <class T>
      SmartPointer<Event> newEvent(int fd, unsigned events, T *obj,
                                   typename EventMemberFunctor<T>::member_t
                                   member) {
        return newEvent(fd, events, new EventMemberFunctor<T>(obj, member));
      }

      template <class T>
      SmartPointer<Event> newSignal(int signal, T *obj,
                                    typename EventMemberFunctor<T>::member_t
                                    member) {
        return newSignal(signal, new EventMemberFunctor<T>(obj, member));
      }


      void dispatch();
      void loopOnce();
      bool loopNonBlock();
      void loopBreak();
      void loopContinue();
      void loopExit();
    };
  }
}

#endif // CB_EVENT_BASE_H

