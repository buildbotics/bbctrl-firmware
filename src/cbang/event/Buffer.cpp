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

#include "Buffer.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <event2/buffer.h>

#include <string.h>


using namespace std;
using namespace cb::Event;


Buffer::Buffer(evbuffer *evb, bool deallocate) :
  evb(evb), deallocate(deallocate) {
}


Buffer::Buffer(const char *data, unsigned length) :
  evb(evbuffer_new()), deallocate(true) {
  if (!evb) THROW("Failed to create event buffer");
  add(data, length);
}


Buffer::Buffer(const char *s) : evb(evbuffer_new()), deallocate(true) {
  if (!evb) THROW("Failed to create event buffer");
  add(s);
}


Buffer::Buffer(const string &s) : evb(evbuffer_new()), deallocate(true) {
  if (!evb) THROW("Failed to create event buffer");
  add(s);
}


Buffer::Buffer() : evb(evbuffer_new()), deallocate(true) {
  if (!evb) THROW("Failed to create event buffer");
}


Buffer::~Buffer() {
  if (evb && deallocate) evbuffer_free(evb);
}


unsigned Buffer::getLength() const {
  return evbuffer_get_length(evb);
}


string Buffer::toString() const {
  return string((char *)evbuffer_pullup(evb, -1), evbuffer_get_length(evb));
}


string Buffer::hexdump() const {
  return
    String::hexdump((char *)evbuffer_pullup(evb, -1), evbuffer_get_length(evb));
}


void Buffer::clear() {
  evbuffer_drain(evb, evbuffer_get_length(evb));
}


void Buffer::add(const char *data, unsigned length) {
  evbuffer_add(evb, data, length);
}


void Buffer::add(const char *s) {
  evbuffer_add(evb, s, strlen(s));
}


void Buffer::add(const string &s) {
#ifndef WIN32
  add(s.data(), s.length());
#else
  add(s.c_str(), s.length());
#endif
}


int Buffer::remove(char *data, unsigned length) {
  return evbuffer_remove(evb, data, length);
}
