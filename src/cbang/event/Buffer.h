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

#ifndef CB_EVENT_BUFFER_H
#define CB_EVENT_BUFFER_H

#include <string>

struct evbuffer;


namespace cb {
  namespace Event {
    class Buffer {
      evbuffer *evb;
      bool deallocate;

    public:
      Buffer(const Buffer &o) : evb(o.evb), deallocate(false) {}
      Buffer(evbuffer *evb, bool deallocate);
      Buffer(const char *data, unsigned length);
      Buffer(const char *s);
      Buffer(const std::string &s);
      Buffer();
      ~Buffer();

      evbuffer *getBuffer() const {return evb;}
      evbuffer *adopt() {deallocate = false; return evb;}

      unsigned getLength() const;
      const char *toCString() const;
      std::string toString() const;
      std::string hexdump() const;

      void clear();

      void add(const Buffer &buf);
      void addRef(const Buffer &buf);
      void add(const char *data, unsigned length);
      void add(const char *s);
      void add(const std::string &s);
      void addFile(const std::string &path);

      int remove(char *data, unsigned length);
    };
  }
}

#endif // CB_EVENT_BUFFER_H

