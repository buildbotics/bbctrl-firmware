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

#include "Handler.h"

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/os/SystemUtilities.h>

#include <ctype.h>
#include <stdarg.h>

#include <sstream>

using namespace std;
using namespace cb;
using namespace cb::Script;


string Handler::eval(const string &s) {
  ostringstream stream;
  eval(Context(*this, stream), s);
  return stream.str();
}


void Handler::eval(ostream &stream, const string &s) {
  eval(Context(*this, stream), s);
}


static char escapeChar(char c) {
  // TODO handle \0### octal codes and \x### hex codes
  switch (c) {
  case '0': return '\0';
  case 'a': return '\a';
  case 'b': return '\b';
  case 'f': return '\f';
  case 'n': return '\n';
  case 'r': return '\r';
  case 't': return '\t';
  case 'v': return '\v';
  default: return c;
  }
}


static const char *parseArgs(Arguments &args, const char *s) {
  bool inArg = false;
  bool inSQuote = false;
  bool inDQuote = false;
  string arg;
  unsigned parenDepth = 0;
  bool closeParen = true;

  for (; *s; s++) {
    switch (*s) {
    case '\\':
      inArg = true;
      arg.push_back('\\'); // Keep escape codes
      if (*++s) arg.push_back(*s);
      continue;

    case ' ':
    case '\t':
    case '\n':
    case '\r':
      if (!inSQuote && !inDQuote && !parenDepth) {
        if (inArg) {
          inArg = false;
          args.push_back(arg);
          arg.clear();
        }

        continue;
      }
      break;

    case '\'':
      if (inSQuote) {
        inSQuote = false;
        if (!parenDepth) continue;

      } else if (!inDQuote) {
        inSQuote = true;
        inArg = true;
        if (!parenDepth) continue;
      }
      break;

    case '"':
      if (inDQuote) {
        inDQuote = false;
        if (!parenDepth) continue;

      } else if (!inSQuote) {
        inDQuote = true;
        inArg = true;
        if (!parenDepth) continue;
      }
      break;

    case '(':
      if (!inSQuote && !inDQuote) {
        if (!parenDepth++) {
          // If the argument is enclosed with () then strip the outside parens
          if (!inArg) {
            inArg = true;
            closeParen = false;
            continue;
          } else closeParen = true;
        }
      }
      break;

    case ')':
      if (!inSQuote && !inDQuote) {
        if (!parenDepth) goto done;
        if (!--parenDepth && !closeParen) continue;
      }
      break;
    }

    inArg = true;
    arg.push_back(*s);
  }

 done:
  if (inArg) args.push_back(arg);

  return s;
}


static const char *parseVar(const Context &ctx, const char *s) {
  const char *start = s++;
  Arguments args;

  if (*s != '(') {
    // Parse name
    if (!(isalpha(*s) || *s == '_')) {
      // Failed
      ctx.stream << *start;
      return s;
    }

    // Names can consist of letters, numbers, '_' or '-'.
    while (*s && (isalnum(*s) || *s == '_' || *s == '-')) s++;

    args.push_back(string(start + 1, (size_t)(s - start - 1)));
  }

  if (*s == '(') {
    // Function
    s = parseArgs(args, s + 1);
    if (*s != ')') THROWS("Expected ')' found '"
                          << (*s ? string(1, *s) : string("null")) << "'");
    ctx.handler.eval(Context(ctx, args));
    return s + 1;
  }

  // Variable
  ctx.handler.eval(Context(ctx, args));

  return s;
}


void Handler::eval(const Context &ctx, const char *s) {
  bool escape = false;

  while (*s) {
    if (escape) {
      if (*s != '\\' && *s != '$') ctx.stream << '\\';
      ctx.stream << escapeChar(*s++);
      escape = false;
      continue;
    }

    switch (*s) {
    case '\\': escape = true; s++; break;
    case '$': s = parseVar(ctx, s); break;
    default: ctx.stream << *s++; break;
    }
  }
}


void Handler::eval(const Context &ctx, const string &s) {
  eval(ctx, s.c_str());
}


void Handler::eval(const Context &ctx, const char *s, unsigned length) {
  eval(ctx, string(s, length));
}


void Handler::evalf(const Context &ctx, const char *s, ...) {
  va_list ap;

  va_start(ap, s);
  eval(ctx, String::vprintf(s, ap));
  va_end(ap);
}


void Handler::parse(Arguments &args, const string &s) {
  parseArgs(args, s.c_str());
}


void Handler::exec(const Context &ctx, const std::string &script) {
  // Allocate space
  uint64_t size = SystemUtilities::getFileSize(script);
  SmartPointer<char>::Array buffer = new char[size + 1];
  buffer[size] = 0;

  // Read it
  SmartPointer<iostream> f = SystemUtilities::open(script, ios::in);
  f->read(buffer.get(), size);
  if (f->fail()) THROWS("Failed to read '" << script << "'");

  // Run it
  eval(ctx, buffer.get());
}


void Handler::exec(ostream &stream, const string &script) {
  exec(Context(*this, stream), script);
}
