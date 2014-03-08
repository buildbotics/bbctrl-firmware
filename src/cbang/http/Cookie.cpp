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

#include "Cookie.h"

#include "Message.h"

#include <cbang/String.h>
#include <cbang/time/Time.h>
#include <cbang/util/StringMap.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


string Cookie::toString() const {
  ostringstream s;
  s << name << '=' << value;

  if (!domain.empty()) s << "; Domain=" << domain;
  if (!path.empty()) s << "; Path=" << path;
  if (expires) s << "; Expires=" << Time(expires, Message::TIME_FORMAT);
  if (maxAge) s << "; Max-Age=" << String(maxAge);
  if (httpOnly) s << "; HttpOnly";
  if (secure) s << "; Secure";

  return s.str();
}


void Cookie::read(const string &s) {
  vector<string> tokens;
  String::tokenize(s, tokens, "; \t\n\r");

  for (unsigned i = 0; i < tokens.size(); i++) {
    size_t pos = tokens[i].find('=');
    string name = tokens[i].substr(0, pos);
    string value = pos == string::npos ? string() : tokens[i].substr(pos + 1);

    if (!i) {
      this->name = name;
      this->value = value;

    } else if (name == "Domain") domain = value;
    else if (name == "Path") path = value;
    else if (name == "Expires")
      expires = Time::parse(value, Message::TIME_FORMAT);
    else if (name == "Max-Age") expires = String::parseU64(value);
    else if (name == "HttpOnly") httpOnly = true;
    else if (name == "Secure") secure = true;
  }
}
