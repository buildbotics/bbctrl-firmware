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

#include "Header.h"

#include "ContentTypes.h"

#include <cbang/String.h>
#include <cbang/Exception.h>

#include <cbang/log/Logger.h>

#include <string.h>

#include <vector>
#include <sstream>
#include <cctype>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


Header::Header(const string &data) {
  read(data);
}


string Header::setContentTypeFromExtension(const string &filename) {
  size_t ptr = filename.find_last_of('.');
  string ext;

  if (ptr != string::npos) ext = filename.substr(ptr + 1);

  ContentTypes::const_iterator it = ContentTypes::instance().find(ext);
  if (it != ContentTypes::instance().end()) setContentType(it->second);
  else setContentType("text/html"); // Default

  return ext;
}


void Header::setContentLength(streamsize length) {
  set("Content-Length", String((uint64_t)length));
}


streamsize Header::getContentLength() const {
  if (!has("Content-Length")) return 0;
  return (streamsize)String::parseU64(get("Content-Length"));
}


void Header::read(const string &data) {
  istringstream stream(data);
  read(stream);
}


istream &Header::read(istream &stream) {
  string key;

  clear();

  while (!stream.fail()) {
    char line[1024];
    memset(line, 0, sizeof(line));

    stream.getline(line, sizeof(line));
    if (stream.fail()) THROW("Failed to read stream");

    LOG_DEBUG(6, "Read " << stream.gcount() << " bytes of HTTP header");

    // Remove trailing space
    for (int i = strlen(line) - 1; 0 <= i && isspace(line[i]); i--)
      line[i] = 0;

    if (line[0] == 0) break; // End of header

    LOG_DEBUG(5, "HTTP::Header: " << line);

    if (stream.eof()) THROW("Reached end of stream before end of HTTP header");

    char *ptr = line;
    if (!key.empty() && isspace(*ptr)) { // Continuation of last line
      while (*ptr && isspace(*ptr)) ptr++;
      set(key, get(key) + " " + ptr);

    } else {
      ptr = strchr(line, ':');
      if (!ptr) THROWS("Invalid  header '" << line << "'");

      *ptr++ = 0;
      key = line;

      while (*ptr && isspace(*ptr)) ptr++;

      // TODO handle repeated keys such as Set-Cookie
      set(key, ptr);
    }
  }

  return stream;
}


ostream &Header::write(ostream &stream) const {
  for (const_iterator it = begin(); it != end(); it++)
    stream << it->first << ": " << it->second << "\r\n";

  return stream;
}


void Header::parseKeyValueList(const string &s, StringMap &map) {
  enum {
    START_STATE,
    KEY_STATE,
    VALUE_START_STATE,
    VALUE_STATE,
    VALUE_ESCAPE_STATE,
  } state = START_STATE;

  string key;
  string value;

  string::const_iterator it = s.begin();
  while (it != s.end()) {
    switch (state) {
    case START_STATE:
      if (*it != ',' && !isspace(*it)) {
        key.clear();
        value.clear();
        state = KEY_STATE;
        continue;
      }
      break;

    case KEY_STATE:
      if (*it == '=') state = VALUE_START_STATE;
      else key += *it;
      break;

    case VALUE_START_STATE:
      state = VALUE_STATE;
      if (*it == '"') break; // Skip opening quote
      // Otherwise drop through

    case VALUE_STATE:
      if (*it == '"' || *it == ',') {
        map[String::toLower(key)] = value;
        state = START_STATE;
        break;
      }
      if (*it == '\\') {
        state = VALUE_ESCAPE_STATE;
        break;
      }

      value += *it;
      break;

    case VALUE_ESCAPE_STATE:
      value += *it;
      state = VALUE_STATE;
      break;
    }

    it++;
  }
}


string Header::quoted(const string &s) {
  string result = "\"";

  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    if ((!isspace(*it) && *it < 32) || *it == 127)
      THROWS("Character " << (int)*it << " not allowed in HTTP quoted string");

    if (*it == '"') result += "\\";
    result += *it;
  }

  return result + "\"";
}
