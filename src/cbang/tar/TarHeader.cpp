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

#include "TarHeader.h"

#include <cbang/Exception.h>

#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>

using namespace std;
using namespace cb;


#ifdef __APPLE__
static size_t __strnlen(const char *s, size_t limit) {
  size_t len = 0;
  while (len < limit && *s++) len++;
  return len;
}
#define strnlen __strnlen
#endif


TarHeader::TarHeader(const string &filename, uint64_t size) {
  setFilename(filename);
  setMode(0644);
  setOwner(0);
  setGroup(0);
  setSize(size);
  setModTime(time(0));
  setType(NORMAL_FILE);
  setLinkName("");
  memset(reserved, 0, 255);
}


void TarHeader::setFilename(const string &filename) {
  checksum_valid = false;
  writeString(filename, this->filename, 100);
}


void TarHeader::setMode(uint32_t mode) {
  checksum_valid = false;
  writeNumber(mode, this->mode, 8);
}


void TarHeader::setOwner(uint32_t owner) {
  checksum_valid = false;
  writeNumber(owner, this->owner, 8);
}


void TarHeader::setGroup(uint32_t group) {
  checksum_valid = false;
  writeNumber(group, this->group, 8);
}


void TarHeader::setSize(uint64_t size) {
  checksum_valid = false;
  writeNumber(size, this->size, 12);
}


void TarHeader::setModTime(uint64_t mod_time) {
  checksum_valid = false;
  writeNumber(mod_time, this->mod_time, 12);
}


void TarHeader::setType(type_t type) {
  checksum_valid = false;
  this->type[0] = (type & 7) + '0';
}


void TarHeader::setLinkName(const string &link_name) {
  checksum_valid = false;
  writeString(link_name, this->link_name, 100);
}


const string TarHeader::getFilename() const {
  return string(filename, strnlen(filename, 100));
}


uint32_t TarHeader::getMode() const {
  return readNumber(mode, 8);
}


uint32_t TarHeader::getOwner() const {
  return readNumber(owner, 8);
}


uint32_t TarHeader::getGroup() const {
  return readNumber(group, 8);
}


uint64_t TarHeader::getSize() const {
  return readNumber(size, 12);
}


uint64_t TarHeader::getModTime() const {
  return readNumber(mod_time, 12);
}


TarHeader::type_t TarHeader::getType() const {
  return (type_t)(type[0] - '0');
}


const string TarHeader::getLinkName() const {
  return string(link_name, strnlen(link_name, 100));
}


unsigned TarHeader::computeChecksum() {
  unsigned sum = 8 * 32;
  const char *ptr = filename;

  for (int i = 0; i < 148; i++)
    sum += *ptr++;

  ptr = type;
  for (int i = 156; i < 512; i++)
    sum += *ptr++;

  return sum;
}


unsigned TarHeader::updateChecksum() {
  unsigned sum = computeChecksum();
  writeNumber(sum, checksum, 7);
  checksum[7] = ' ';

  checksum_valid = true;

  return sum;
}


bool TarHeader::read(istream &stream) {
  stream.read(filename, 512);
  if (stream.gcount() != 512) return false;

  unsigned sum = readNumber(checksum, 6);
  if (sum != 0) {
    unsigned calculated = computeChecksum();
    if (sum != calculated)
      THROWS("Invalid checksum in tar header calculated=" << calculated
             << " expected=" << sum);
  }

  return true;
}


void TarHeader::write(ostream &stream) {
  if (!checksum_valid) updateChecksum();

  stream.write(filename, 512);
}


void TarHeader::writeNumber(uint64_t n, char *buf, uint32_t length) {
  sprintf(buf, "%0*llo", length - 1, (unsigned long long)n);
}


bool TarHeader::isEOF() const {
  return readNumber(checksum, 6) == 0;
}


void TarHeader::writeString(const string &s, char *buf, uint32_t length) {
  strncpy(buf, s.c_str(), length);
  for (unsigned i = s.length(); i < length; i++)
    buf[i] = 0;
}


uint64_t TarHeader::readNumber(const char *buf, uint32_t length) {
  uint64_t value = 0;

  for (unsigned i = 0; i < length; i++) {
    if ('0' <= buf[i] && buf[i] <= '7')
      value = (value << 3) | (buf[i] - '0');
    else if (buf[i] == ' ' || buf[i] == 0) break;
    else THROWS("Error converting number '" << string(buf, length) << "'");
  }

  return value;
}
