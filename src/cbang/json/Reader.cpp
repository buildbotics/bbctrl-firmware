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

#include "Reader.h"

#include "Null.h"
#include "Boolean.h"
#include "Number.h"
#include "String.h"
#include "List.h"
#include "Dict.h"
#include "Builder.h"

#include <cbang/String.h>

#include <vector>
#include <cctype>
#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::JSON;


void Reader::parse(Sync &sync) {
  while (good()) {
    switch (next()) {
    case 'N': case 'n':
      parseNull();
      return sync.writeNull();

    case 'T': case 't': case 'F': case 'f':
      return sync.writeBoolean(parseBoolean());

    case '-': case '.':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      return sync.write(parseNumber());

    case '"':
      return sync.write(parseString());

    case '[':
      sync.beginList();
      parseList(sync);
      sync.endList();
      return;

    case '{':
      sync.beginDict();
      parseDict(sync);
      sync.endDict();
      return;

    default: match("NnTtFf-.0123456789\"[{");
    }
  }

  error("Unexpected end of expression");
  throw "Unreachable";
}


ValuePtr Reader::parse() {
  Builder builder;
  parse(builder);
  return builder.getRoot();
}


SmartPointer<Value> Reader::parse(const InputSource &src) {
  return Reader(src).parse();
}


char Reader::get() {
  char c = stream.get();

  if (c == '\n') {
    line++;
    column = 0;

  } else if (c != '\r') column++;

  return c;
}


char Reader::next() {
  while (good())
    switch (stream.peek()) {
    case '\n': case '\r': case '\t': case ' ': get(); break;

    case '#':
      while (stream.peek() != '\n') get();
      break;

    default: return stream.peek();
    }

  error("Unexpected end of expression");
  throw "Unreachable";
}


bool Reader::tryMatch(char c) {
  if (c == next()) {
    advance();
    return true;
  }

  return false;
}


char Reader::match(const char *chars) {
  char x = next();

  for (int i = 0; chars[i]; i++)
    if (x == chars[i]) {
      advance();
      return x;
    }

  error(SSTR("Expected one of '" << cb::String::escapeC(chars)
             << "' but found '" << cb::String::escapeC(string(1, x)) << '\''));
  throw "Unreachable";
}


const string Reader::parseKeyword() {
  string s;
  while (good() && isalpha(stream.peek())) s += stream.get();
  return s;
}


void Reader::parseNull() {
  string value = cb::String::toLower(parseKeyword());

  if (value != "none" && value != "null")
    error(SSTR("Expected keyword 'None' or 'null' but found '" << value
               << '\''));
}


bool Reader::parseBoolean() {
  string value = cb::String::toLower(parseKeyword());

  if (value == "true") return true;
  else if (value == "false") return false;

  error(SSTR("Expected keyword 'true' or 'false' but found '" << value
             << '\''));
  throw "Unreachable";
}


double Reader::parseNumber() {
  string value;

  while (good()) {
    char c = next();

    switch (c) {
    case 'e': case 'E': case '-': case '.':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      value += c;
      advance();
      continue;
    }

    break;
  }

  return cb::String::parseDouble(value);
}


const string Reader::parseString() {
  match("\"");

  string s;
  bool escape = false;
  while (good()) {
    char c = get();

    if (c == '"' && !escape) break;
    if (good()) s += c;

    escape = !escape && c == '\\';
  }

  return unescape(s);
}


void Reader::parseList(Sync &sync) {
  match("[");

  while (good()) {
    if (tryMatch(']')) return; // End or trailing comma

    sync.beginAppend();
    parse(sync);

    if (match(",]") == ']') return; // Continuation or end
  }
}


void Reader::parseDict(Sync &sync) {
  match("{");

  while (good()) {
    if (tryMatch('}')) return; // Empty or trailing comma

    string key = parseString();
    match(":");
    sync.beginInsert(key);
    parse(sync);

    if (match(",}") == '}') return; // Continuation or end
  }
}


void Reader::error(const string &msg) const {
  THROWS('@' << src << ':' << line << ':' << column << ' ' << msg);
}


string Reader::unescape(const string &s) {
  return cb::String::unescapeC(s);
}
