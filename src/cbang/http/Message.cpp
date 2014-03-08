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

#include "Message.h"

#include <cbang/String.h>
#include <cbang/log/Logger.h>

#include <sstream>

#include <string.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;

const char *Message::TIME_FORMAT = "%a, %d %b %Y %H:%M:%S %Z";


bool Message::isChunked() const {
  string encoding = get("Transfer-Encoding", "");

  vector<string> parts;
  String::tokenize(encoding, parts, ",");
  for (unsigned i = 0; i < parts.size(); i++)
    if (String::toLower(String::trim(parts[i])) == "chunked") return true;

  return false;
}


void Message::setChunked(bool chunked) {
  if (chunked != isChunked()) {
    string encoding = String::trim(get("Transfer-Encoding", ""));

    if (chunked) { // Add it
      if (!encoding.empty()) encoding += ", ";
      encoding += "chunked";

    } else { // Remove it
      vector<string> parts;
      String::tokenize(encoding, parts, ",");
      encoding.clear();
      for (unsigned i = 0; i < parts.size(); i++)
        if (String::toLower(String::trim(parts[i])) != "chunked") {
          if (!encoding.empty()) encoding += ", ";
          encoding += parts[i];
        }
    }

    if (encoding.empty()) unset("Transfer-Encoding");
    else set("Transfer-Encoding", encoding);
  }
}


bool Message::hasCookie(const string &name) const {
  if (!has("Cookie")) return false;

  vector<string> cookies;
  String::tokenize(get("Cookie"), cookies, "; \t\n\r");

  for (unsigned i = 0; i < cookies.size(); i++)
    if (name == cookies[i].substr(0, cookies[i].find('='))) return true;

  return false;
}


string Message::getCookie(const string &name) const {
  if (has("Cookie")) {
    // Return only the first matching cookie
    vector<string> cookies;
    String::tokenize(get("Cookie"), cookies, "; \t\n\r");

    for (unsigned i = 0; i < cookies.size(); i++) {
      size_t pos = cookies[i].find('=');

      if (name == cookies[i].substr(0, pos))
        return pos == string::npos ? string() : cookies[i].substr(pos + 1);
    }
  }

  THROWS("Cookie '" << name << "' not set");
}


string Message::findCookie(const string &name) const {
  try {
    return getCookie(name);
  } catch (const Exception &e) {
    return string();
  }
}


void Message::setPersistent(bool x) {
  if (getVersion() < 1.1) {
    if (x) set("Connection", "Keep-Alive");
    else erase("Connection");

  } else if (x) erase("Connection");
  else set("Connection", "close");
}


bool Message::isPersistent() const {
  string connection = String::toLower(get("Connection", ""));
  if (getVersion() < 1.1) return connection == "keep-alive";
  return connection != "close";
}


string Message::getHeader() const {
  ostringstream str;
  str << *this;
  return str.str();
}


string Message::getVersionString() const {
  return String::printf("HTTP/%.01f", version);
}


void Message::readVersionString(const string &s) {
  if (s.length() < 5 || s.substr(0, 5) != "HTTP/")
    THROWS("Missing 'HTTP/' in HTTP-Version '" << s << "'");

  setVersion(String::parseDouble(s.substr(5)));
}


void Message::read(const string &data) {
  istringstream stream(data);
  read(stream);
}


istream &Message::read(istream &stream) {
  char line[1024];
  memset(line, 0, sizeof(line));

  stream.getline(line, sizeof(line));
  if (stream.fail()) THROW("Failed to read stream");
  LOG_DEBUG(6, "Read " << stream.gcount() << " bytes of HTTP header line");

  // Remove trailing space
  int len = strlen(line);
  for (int i = len - 1; 0 <= i && isspace(line[i]); i--)
    line[i] = 0;

  LOG_DEBUG(5, "HTTP header line '" << line << "' length " << len);

  readHeaderLine(line);
  return Header::read(stream);
}


ostream &Message::write(ostream &stream) const {
  stream << getHeaderLine() << "\r\n";

  Header::write(stream);

  return stream << "\r\n";
}
