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

#include "Packet.h"

#include <cbang/Exception.h>

#include <cbang/db/Blob.h>

#include <stdlib.h>
#include <algorithm>

using namespace std;
using namespace cb;


Packet::Packet() :
  data((char *)calloc(4096, 1)), size(4096), deallocate(true) {
  if (!data) THROW("Failed to allocate memory");
}


Packet::Packet(unsigned size) :
  data(0), size(size), deallocate(true) {
  if (size) {
    data = (char *)calloc(size, 1);
    if (!data) THROW("Failed to allocate memory");
  }
}


Packet::Packet(char *data, unsigned size, bool deallocate) :
  data(data), size(size), deallocate(deallocate) {}



Packet::Packet(const string &s) :
  data((char *)malloc(s.length())), size(s.length()), deallocate(true) {
  memcpy(data, s.c_str(), size);
}


Packet::Packet(const Packet &o) : data(0), size(o.size), deallocate(true) {
  data = (char *)malloc(size);
  memcpy(data, o.data, size);
}


Packet::Packet(Packet &o, bool steal) :
  data(0), size(o.size), deallocate(true) {
  if (steal) this->steal(o);
  else {
    data = (char *)malloc(size);
    memcpy(data, o.data, size);
  }
}


Packet::Packet(const DB::Blob &blob) :
  data((char *)malloc(blob.getLength())), size(blob.getLength()),
  deallocate(true) {
  memcpy(data, blob.getData(), size);
}


Packet::~Packet() {
  if (deallocate && data) {
    free(data);
    data = 0;
  }
}


Packet &Packet::operator=(const Packet &o) {
  if (data && deallocate) free(data);
  data = (char *)malloc(o.size);
  size = o.size;
  memcpy(data, o.data, size);
  deallocate = true;
  return *this;
}


void Packet::set(char *data, unsigned size, bool deallocate) {
  if (this->data && this->deallocate) free(this->data);
  this->data = data;
  this->size = size;
  this->deallocate = deallocate;
}


void Packet::steal(Packet &packet) {
  if (data && deallocate) free(data);
  data = packet.data;
  size = packet.size;
  deallocate = packet.deallocate;
  packet.deallocate = false;
}


void Packet::copy(Packet &packet) {
  memcpy(getData(), packet.getData(), min(getSize(), packet.getSize()));
}


void Packet::increase(unsigned capacity) {
  if (capacity && capacity < size) return;

  char *newData = deallocate ? data : 0;
  unsigned newSize;
  if (capacity) newSize = capacity;
  else newSize = size < 2048 ? 4096 : size * 2;

  // Allocate
  if (!(newData = (char *)realloc(newData, newSize)))
    THROWS("Failed to allocate " << newSize << " bytes.");

  // If this was a new allocation then copy old data
  if (!deallocate) memcpy(newData, data, size);

  // Clear new memory
  memset(newData + size, 0, newSize - size);

  // Commit
  data = newData;
  size = newSize;
  deallocate = true;
}
