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

#ifndef CB_EVENT_BASE_H
#define CB_EVENT_BASE_H

#include "EventCallback.h"

#include <cbang/SmartPointer.h>

struct event_base;


namespace cb {
  namespace Event {
    class Event;
    class EventCallback;

    class Base : public EventFlag {
      event_base *base;

    public:
      Base();
      ~Base();

      struct event_base *getBase() const {return base;}

      void initPriority(int num);

      SmartPointer<Event>
      newPersistentEvent(const SmartPointer<EventCallback> &cb);
      Event &newEvent(const SmartPointer<EventCallback> &cb);
      Event &newEvent(int fd, unsigned events,
                      const SmartPointer<EventCallback> &cb);
      Event &newSignal(int signal, const SmartPointer<EventCallback> &cb);

      template <class T> SmartPointer<Event>
      newPersistentEvent(T *obj,
                         typename EventMemberFunctor<T>::member_t member) {
        return newPersistentEvent(new EventMemberFunctor<T>(obj, member));
      }

      template <class T>
      Event &newEvent(T *obj, typename EventMemberFunctor<T>::member_t member) {
        return newEvent(new EventMemberFunctor<T>(obj, member));
      }

      template <class T>
      Event &newEvent(int fd, unsigned events, T *obj,
                      typename EventMemberFunctor<T>::member_t member) {
        return newEvent(fd, events, new EventMemberFunctor<T>(obj, member));
      }

      template <class T>
      Event &newSignal(int signal, T *obj,
                       typename EventMemberFunctor<T>::member_t member) {
        return newSignal(signal, new EventMemberFunctor<T>(obj, member));
      }

      void dispatch();
      void loop();
      void loopOnce();
      bool loopNonBlock();
      void loopBreak();
      void loopContinue();
      void loopExit();
    };
  }
}

#endif // CB_EVENT_BASE_H

