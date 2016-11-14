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

#include "Signature.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cctype>

using namespace cb::js;
using namespace cb;
using namespace std;


string Signature::toString() const {
  string s = name + "(";

  for (unsigned i = 0; i < size(); i++) {
    const string &key = keyAt(i);
    const Value &value = *get(i);

    if (i) s += ", ";
    s += key;
    if (!value.isUndefined()) s += "=" + value.toString();
  }

  if (variable) {
    if (!empty()) s += ", ";
    s += "...";
  }

  return s + ")";
}


bool Signature::isNameStartChar(char c) {
  // TODO This does not support full Unicode
  return isalpha(c) || c == '_' || c == '$';
}


bool Signature::isNameChar(char c) {
  // TODO This does not support full Unicode
  return isNameStartChar(c) || isdigit(c);
}


void Signature::parse(const string &sig) {
  name.clear();
  unsigned state = 0;
  string::const_iterator it = sig.begin();
  string args;

  while (true) {
    switch (state) {
    case 0:
      if (it == sig.end()) invalidEnd("name");
      if (isspace(*it)) break;
      if (!isNameStartChar(*it)) invalidChar(*it, "name start character");

      state = 1;
      // Fall through

    case 1: // In name
      if (it != sig.end() && isNameChar(*it)) {
        name.append(1, *it);
        break;
      }

      // Note, if empty cannot be end of string
      if (name.empty()) invalidChar(*it, "name character");

      state = 2;
      // Fall through

    case 2: // Before (
      if (it == sig.end()) invalidEnd("'('");
      if (isspace(*it)) break;
      if (*it != '(') invalidChar(*it, "'('");

      state = 3;
      break;

    case 3: // Args
      if (it == sig.end()) invalidEnd("')'");
      if (*it != ')') {args.append(1, *it); break;}

      parseArgs(args);

      state = 4;
      break;

    case 4: // After )
      if (it == sig.end()) return;
      if (isspace(*it)) break;

      invalidChar(*it, "end of signature");
    }

    it++;
  }
}


void Signature::parseArgs(const string &args) {
  string name;
  string value;
  unsigned state = 0;
  bool hasDot = false;
  bool hasMinus = false;
  bool hasDigit = false;
  bool escape = false;
  char quote = 0;

  clear();
  variable = false;
  string::const_iterator it = args.begin();

  while (true) {
    switch (state) {
    case 0: // Before name
      if (it == args.end()) return;
      if (isspace(*it)) break;
      if (*it == '.') {state = 9; break;}
      if (!isNameStartChar(*it)) invalidChar(*it, "name start character");

      state = 1;
      // Fall through

    case 1: // In name
      if (it != args.end() && isNameChar(*it)) {
        name.append(1, *it);
        break;
      }

      // Note, if empty cannot be end of string
      if (name.empty()) invalidChar(*it, "name character");

      state = 2;
      // Fall through

    case 2: // After name
      if (it == args.end() || *it == ',') {
        insertUndefined(name);
        state = 7;
        continue;
      }
      if (*it == '=') {state = 3; break;}
      if (isspace(*it)) break;

      invalidChar(*it, "',', '=' or space after name");

    case 3: // Start value
      if (it == args.end()) invalidEnd("value");

      if (*it == '\'' || *it == '"') {quote = *it; state = 4; break;}
      if (isdigit(*it) || *it == '-' || *it == '.') {state = 5; continue;}
      if (isalpha(*it)) {state = 6; continue;}

      invalidChar(*it, "''', '\"', '-', '.', digit or keyword after '='");

    case 4: // String
      if (it == args.end()) invalidEnd(SSTR("'" << quote << "'"));

      if (escape || (*it != quote && *it != '\\')) {
        escape = false;
        value.append(1, *it);

      } else if (*it == '\\') escape = true;
      else { // *it == quote
        insert(name, value);
        state = 7;
      }
      break;

    case 5: // Number
      if (it != args.end()) {
        if (value.empty() && *it == '-') {
          hasMinus = true;
          value.append(1, '-');
          break;
        }
        if (!hasDot && *it == '.') {
          hasDot = true;
          value.append(1, '.');
          break;
        }
        if (isdigit(*it)) {hasDigit = true; value.append(1, *it); break;}
      }

      if (!hasDigit) THROWS("Invalid number '" << value << "' in signature");

      if (hasDot) insert(name, String::parseDouble(value));
      else if (hasMinus) insert(name, String::parseS32(value));
      else insert(name, String::parseU32(value));

      state = 7;
      continue;

    case 6: // Keyword
      if (it != args.end() && isalpha(*it)) {value.append(1, *it); break;}

      if (value == "undefined") insertUndefined(name);
      else if (value == "null") insertNull(name);
      else if (value == "true") insertBoolean(name, true);
      else if (value == "false") insertBoolean(name, false);
      else THROWS("Invalid keyword '" << value << "' in signature");

      state = 7;
      // Fall through

    case 7: // End of arg
      if (it == args.end()) return;

      if (isspace(*it)) break;
      if (*it == ',') {
        name.clear();
        value.clear();
        escape = hasDigit = hasDot = hasMinus = false;

        state = 8;
        break;
      }

      invalidChar(*it, "space, ',' or end of signature");

    case 8: // Before next arg
      if (it == args.end()) invalidEnd("argument definition after ','");

      if (isspace(*it)) break;

      state = 0;
      continue;

    case 9: // Ellipsis
    case 10:
      if (it == args.end()) invalidEnd("ellipsis");
      if (*it != '.') invalidChar(*it, "ellipsis");

      if (state++ == 10) variable = true;
      break;

    case 11: // After ellipsis
      if (it == args.end()) return;
      if (isspace(*it)) break;
      invalidChar(*it, "end of signature after ellipsis");
    }

    it++;
  }
}


void Signature::invalidChar(char c, const string &expected) {
  THROWS("Invalid character '" << c << "' in signature, expected " << expected);
}


void Signature::invalidEnd(const string &expected) {
  THROWS("End of signature, expected " << expected);
}
