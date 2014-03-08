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

#include "XMLWriter.h"

#include <ctype.h>

using namespace std;
using namespace cb;


void XMLWriter::entityRef(const string &name) {
  stream << '&' << name << ';';
  startOfLine = false;
}


void XMLWriter::startElement(const string &name, const std::string &attrs) {
  startElement(name, XMLAttributes(attrs));
}


void XMLWriter::startElement(const string &name, const XMLAttributes &attrs) {
  startOfLine = false; // Always start element on a new line

  if (!closed) stream << '>';
  if (depth) wrap();

  stream << '<' << escape(name);

  XMLAttributes::const_iterator it;
  for (it = attrs.begin(); it != attrs.end(); it++)
    stream << ' ' << escape(it->first) << "='" << escape(it->second) << '\'';

  closed = false;
  depth++;
}


void XMLWriter::endElement(const string &name) {
  depth--;

  if (!closed) {
    stream << "/>";
    closed = true;

  } else {
    wrap();
    stream << "</" << escape(name) << '>';
  }

  startOfLine = false;
}


void XMLWriter::text(const string &text) {
  if (!text.length()) return;

  string::const_iterator it = text.begin();

  if (!closed) {
    stream << '>';
    closed = true;
    startOfLine = false;

    if (pretty) while (it != text.end() && isspace(*it)) it++;
    wrap();
  }

  for (; it != text.end(); it++) {
    switch (*it) {
    case '<': stream << "&lt;"; break;
    case '>': stream << "&gt;"; break;
    case '&': stream << "&amp;"; break;
    default: stream << *it; break;
    }
    startOfLine = *it == '\n' || *it == '\r';
  }

  // TODO wrap lines at 80 columns in pretty print mode?
}


void XMLWriter::comment(const string &text) {
  startOfLine = false; // Always start comment on a new line

  if (!closed) stream << '>';

  wrap();
  stream << "<!-- ";

  bool dash = false;
  for (string::const_iterator it = text.begin(); it != text.end(); it++) {
    if (*it == '-') {
      if (dash) {
        // Drop double dash
        stream.put(' ');
        dash = false;
        continue;
      }
      dash = true;
    } else dash = false;

    switch (*it) {
    case '\r': break;
#ifdef _WIN32
    case '\n': stream.put('\r');
#endif
    default: stream.put(*it);
    }
  }

  stream << " -->";
  closed = true;
  startOfLine = false;
}


void XMLWriter::indent() {
  if (pretty) {
    for (unsigned i = 0; i < depth; i++)
      stream << "  ";
    startOfLine = false;
  }
}


void XMLWriter::wrap() {
  if (pretty) {
    if (!startOfLine) {
#ifdef _WIN32
      stream << "\r\n";
#else
      stream << '\n';
#endif
    }

    indent();
  }
}


const string XMLWriter::escape(const string &name) {
  string result;

  for (string::const_iterator it = name.begin(); it != name.end(); it++)
    switch (*it) {
    case '<': result += "&lt;"; break;
    case '>': result += "&gt;"; break;
    case '&': result += "&amp;"; break;
    case '"': result += "&quot;"; break;
    case '\'': result += "&apos;"; break;
    case '\r': break;
#ifdef _WIN32
    case '\n': result += "\r\n"; break;
#endif
    default: result += *it; break;
    }

  return result;
}


void XMLWriter::simpleElement(const string &name, const string &content,
                              const XMLAttributes &attrs) {
  startElement(name, attrs);
  text(content);
  endElement(name);
}
