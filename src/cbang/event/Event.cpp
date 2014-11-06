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

#include "Event.h"
#include "EventCallback.h"

#include <cbang/util/DefaultCatch.h>
#include <cbang/time/Timer.h>

#include <event2/event.h>

using namespace cb::Event;


namespace {
  void event_cb(evutil_socket_t fd, short flags, void *arg) {
    ((Event *)arg)->call(fd, flags);
  }
}


Event::Event(Base &base, int fd, unsigned events,
             const SmartPointer<EventCallback> &cb, bool selfDestruct) :
  e(event_new(base.getBase(), fd, events, event_cb, this)), cb(cb),
  selfDestruct(selfDestruct) {
  if (!e) THROW("Failed to create event");
}


Event::Event(Base &base, int signal, const SmartPointer<EventCallback> &cb,
             bool selfDestruct) :
  e(event_new(base.getBase(), signal, EV_SIGNAL, event_cb, this)), cb(cb),
  selfDestruct(selfDestruct) {
  if (!e) THROW("Failed to create signal event");
}


Event::~Event() {
  if (e) event_free(e);
}


bool Event::isPending(unsigned events) const {
  return event_pending(e, events, 0);
}


void Event::assign(Base &base, int fd, unsigned events,
                   const SmartPointer<EventCallback> &cb) {
  event_del(e);
  event_assign(e, base.getBase(), fd, events, event_cb, this);
}


void Event::renew(int fd, unsigned events) {
  event_del(e);
  event_assign(e, event_get_base(e), fd, events, event_cb, this);
}


void Event::renew(int signal) {
  event_del(e);
  event_assign(e, event_get_base(e), signal, EV_SIGNAL, event_cb, this);
}


void Event::add(double t) {
  struct timeval tv = Timer::toTimeVal(t);
  event_add(e, &tv);
}


void Event::add() {
  event_add(e, 0);
}


void Event::del() {
  event_del(e);
}


void Event::call(int fd, short flags) {
  try {
    (*cb)(*this, fd, flags);
  } CATCH_ERROR;

  if (selfDestruct && !isPending()) delete this;
}
