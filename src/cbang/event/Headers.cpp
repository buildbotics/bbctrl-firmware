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

#include "Headers.h"

#include <cbang/Exception.h>
#include <cbang/http/ContentTypes.h>

#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <sys/queue.h>

using namespace cb::Event;
using namespace std;


void Headers::clear() {
  evhttp_clear_headers(hdrs);
}


void Headers::add(const string &key, const string &value) {
  evhttp_add_header(hdrs, key.c_str(), value.c_str());
}


void Headers::set(const string &key, const string &value) {
  if (has(key)) remove(key);
  add(key, value);
}


bool Headers::has(const string &key) const {
  return evhttp_find_header(hdrs, key.c_str());
}


string Headers::find(const string &key) const {
  const char *value = evhttp_find_header(hdrs, key.c_str());
  return value ? value : "";
}


string Headers::get(const string &key) const {
  const char *value = evhttp_find_header(hdrs, key.c_str());
  if (!value) THROWS("Header '" << key << "' not found");
  return value;
}


void Headers::remove(const string &key) {
  evhttp_remove_header(hdrs, key.c_str());
}


string Headers::getContentType() const {
  return find("Content-Type");
}


void Headers::setContentType(const string &contentType) {
  set("Content-Type", contentType);
}


void Headers::guessContentType(const std::string &ext) {
  HTTP::ContentTypes::const_iterator it =
    HTTP::ContentTypes::instance().find(ext);
  if (it != HTTP::ContentTypes::instance().end()) setContentType(it->second);
}


void Headers::write(ostream &stream) const {
  for (evkeyval *hdr = TAILQ_FIRST(hdrs); hdr; hdr = TAILQ_NEXT(hdr, next))
    stream << hdr->key << ": " << hdr->value << "\n";
}
