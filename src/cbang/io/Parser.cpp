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

#include "Parser.h"

#include <cbang/Exception.h>
#include <cbang/SStream.h>

using namespace std;
using namespace cb;


string Parser::get() {
  if (current.empty()) {
    // Find start
    while (true) {
      char c = next();
      if (!stream.good() || delims.find(c) == string::npos) {
        current.append(1, c);
        break;
      }
    }

    // Find end
    while (true) {
      char c = next();
      if (!stream.good() || delims.find(c) != string::npos) break;
      current.append(1, c);
    }
  }

  return current;
}


string Parser::advance() {
  string token = get();
  current.clear();
  return token;
}


bool Parser::check(const string &token) {
  return caseSensitive ?
    (get() == token) : (String::toLower(get()) == String::toLower(token));
}


void Parser::match(const string &token) {
  if (!check(token))
    THROWS("Expected '" << token << "' but found " <<
           (current.empty() ? string("end of stream") :
            SSTR("'" << current << '"')));
  current.clear();
}


char Parser::next() {
  char c = stream.get();
  if (stream.good()) {
    if (line == -1) line = 0;
    if (col == -1) col = 0;

    if (c == '\n') {
      line++;
      col = 0;

    } else col++;
  }

  return c;
}
