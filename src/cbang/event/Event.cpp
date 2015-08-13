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

#include "Event.h"
#include "EventCallback.h"

#include <cbang/util/DefaultCatch.h>
#include <cbang/time/Timer.h>
#include <cbang/log/Logger.h>

#include <event2/event.h>
#include <event2/event_struct.h>

using namespace cb::Event;


namespace {
  void event_cb(evutil_socket_t fd, short flags, void *arg) {
    ((Event *)arg)->call(fd, flags);
  }
}


Event::Event(Base &base, int fd, unsigned events,
             const cb::SmartPointer<EventCallback> &cb, bool selfDestruct) :
  e(event_new(base.getBase(), fd, events, event_cb, this)), cb(cb),
  selfDestruct(selfDestruct) {
  LOG_DEBUG(5, "Created new event with fd=" << fd);
  if (!e) THROW("Failed to create event");
}


Event::Event(Base &base, int signal, const cb::SmartPointer<EventCallback> &cb,
             bool selfDestruct) :
  e(event_new(base.getBase(), signal, EV_SIGNAL, event_cb, this)), cb(cb),
  selfDestruct(selfDestruct) {
  LOG_DEBUG(5, "Created new event with signal=" << signal);
  if (!e) THROW("Failed to create signal event");
}


Event::~Event() {
  LOG_DEBUG(5, "~Event()");
  if (e) event_free(e);
}


bool Event::isPending(unsigned events) const {
  return event_pending(e, events, 0);
}


unsigned Event::getEvents() const {
  return event_get_events(e);
}


int Event::getFD() const {
  return event_get_fd(e);
}


void Event::setPriority(int priority) {
  if (event_priority_set(e, priority)) THROW("Failed to set event priority");
}


void Event::assign(Base &base, int fd, unsigned events,
                   const cb::SmartPointer<EventCallback> &cb) {
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


void Event::readd() {
  struct timeval tv = e->ev_timeout;
  event_add(e, &tv);
}


void Event::del() {
  event_del(e);
}


void Event::activate(int flags) {
  event_active(e, flags, 0);
}


void Event::call(int fd, short flags) {
  LOG_DEBUG(5, "Event callback fd=" << fd << " flags=" << flags);

  try {
    (*cb)(*this, fd, flags);
  } CATCH_ERROR;

  if (selfDestruct && !isPending()) delete this;
}


void Event::enableDebugMode() {
  event_enable_debug_mode();
}


namespace {
  static int event_log_level = 0;

  void log_cb(int severity, const char *msg) {
    switch (severity) {
    case _EVENT_LOG_DEBUG:
      LOG_DEBUG(event_log_level, "Event: " << msg);
      break;

    case _EVENT_LOG_MSG:
      LOG_DEBUG(event_log_level, "Event: " << msg);
      break;

    case _EVENT_LOG_WARN:
      LOG_WARNING("Event: " << msg);
      break;

    case _EVENT_LOG_ERR:
      LOG_ERROR("Event: " << msg);
      break;
    }
  }
}


void Event::enableLogging(int level) {
  event_log_level = level;
  event_set_log_callback(log_cb);
}


void Event::enableDebugLogging() {
  event_enable_debug_logging(EVENT_DBG_ALL);
}
