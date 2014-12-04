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

#include "String.h"

#include "Math.h"
#include "SStream.h"
#include "Exception.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

#include <algorithm>
#include <limits>
#include <locale>

#include <boost/regex.hpp>

using namespace std;
using namespace cb;

#ifdef _WIN32
#define strtoll(p, e, b) _strtoi64(p, e, b)
#define strtoull(p, e, b) _strtoui64(p, e, b)
#define strtof(p, e) (float)strtod(p, e)
#define vsnprintf _vsnprintf
#define __va_copy(x, y) (x = y)
#endif

const string String::DEFAULT_DELIMS = " \t\n\r";
const string String::DEFAULT_LINE_DELIMS = "\n";
const string String::LETTERS_LOWER_CASE = "abcdefghijklmnopqrstuvwxyz";
const string String::LETTERS_UPPER_CASE = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const string String::LETTERS =
  String::LETTERS_LOWER_CASE + String::LETTERS_UPPER_CASE;
const string String::NUMBERS = "0123456789";
const string String::ALPHA_NUMERIC = String::LETTERS + String::NUMBERS;


String::String(const char *s, size_type n) {
  size_type len;
  for (len = 0; len < n && s[len]; len++) continue;

  if (len) assign(s, len);
}


String::String(int32_t x)   : string(printf("%i", x)) {}
String::String(uint32_t x)  : string(printf("%u", x)) {}
String::String(int64_t x)   : string(printf("%lli", (long long int)x)) {}
String::String(uint64_t x)  : string(printf("%llu", (long long unsigned)x)) {}
String::String(uint128_t x) : string(SSTR(x)) {}


String::String(double x) : string(printf("%f", x)) {
  int chop = 0;
  char point = use_facet<numpunct<char> >(cout.getloc()).decimal_point();

  for (reverse_iterator it = rbegin(); it != rend(); it++)
    if (*it == '0' || *it == point) {
      chop++;
      if (*it == point) break;

    } else break;

  if (chop) *this = substr(0, length() - chop);
  if (*this == "-0") *this = "0";
}


String::String(bool x) : string(x ? "true" : "false") {}


string String::printf(const char *format, ...) {
  va_list ap;

  va_start(ap, format);
  string result = vprintf(format, ap);
  va_end(ap);

  return result;
}


string String::vprintf(const char *format, va_list ap) {
  va_list copy;
  __va_copy(copy, ap);

  int length = vsnprintf(0, 0, format, copy);
  va_end(copy);

  if (length < 0) THROWS("String format '" << format << "' invalid");

  SmartPointer<char>::Array result = new char[length + 1];

  int ret = vsnprintf(result.get(), length + 1, format, ap);

  if (ret != length) THROWS("String format '" << format << "' failed");

  return result.get();
}


unsigned String::tokenize(const string &s, vector<string> &tokens,
                          const string &delims, bool allowEmpty) {
  size_t i = 0;
  unsigned count = 0;

  while (true) {
    size_t lastEnd = i;
    if ((i = s.find_first_not_of(delims, i)) == string::npos) i = s.length();

    if (allowEmpty)
      for (size_t j = lastEnd + 1; j < i; j++) {
        tokens.push_back(string());
        count++;
      }

    if (i == s.length()) return count;

    size_t end = s.find_first_of(delims, i);
    if (end == string::npos) end = s.length();

    tokens.push_back(s.substr(i, end - i));
    count++;

    i = end;
  }

  return count;
}


unsigned String::tokenizeLine(istream &stream, vector<string> &tokens,
                              const string &delims, const string &lineDelims,
                              unsigned maxLength) {
  string s;

  while (true) {
    char c = stream.get();

    if (!stream.good() || lineDelims.find(c) != string::npos) break;

    s.append(1, c);

    if (maxLength && s.length() == maxLength) break;
  }

  return tokenize(s, tokens, delims);
}


uint8_t String::parseU8(const string &s) {
  uint32_t v = parseU32(s);
  if (v > 255) THROWS("Unsigned 8-bit value '" << s << "' out of range");

  return (uint8_t)v;
}


int8_t String::parseS8(const string &s) {
  int32_t v = parseS32(s);
  if (v < -127 || 127 < v)
    THROWS("Signed 8-bit value '" << s << "' out of range");

  return (int8_t)v;
}


uint16_t String::parseU16(const string &s) {
  uint32_t v = parseU32(s);
  if (65535 < v) THROWS("Unsigned 16-bit value '" << s << "' out of range");

  return (uint16_t)v;
}


int16_t String::parseS16(const string &s) {
  int32_t v = parseS32(s);
  if (v < -32767 || 32767 < v)
    THROWS("Signed 16-bit value '" << s << "' out of range");

  return (int16_t)v;
}


uint32_t String::parseU32(const string &s) {
  errno = 0;
  unsigned long v = strtoul(s.c_str(), 0, 0);
  if (errno || numeric_limits<uint32_t>::max() < v)
    THROWS("Invalid unsigned 32-bit value '" << s << "'");

  return (uint32_t)v;
}


int32_t String::parseS32(const string &s) {
  errno = 0;
  long v = strtol(s.c_str(), 0, 0);
  if (errno|| v < -numeric_limits<int32_t>::max() ||
      numeric_limits<int32_t>::max() < v)
    THROWS("Invalid signed 32-bit value '" << s << "'");

  return (int32_t)v;
}


uint64_t String::parseU64(const string &s) {
  errno = 0;
  unsigned long long v = strtoull(s.c_str(), 0, 0);
  if (errno) THROWS("Invalid unsigned 64-bit value '" << s << "'");

  return (uint64_t)v;
}


int64_t String::parseS64(const string &s) {
  errno = 0;
  long long int v = strtoll(s.c_str(), 0, 0);
  if (errno) THROWS("Invalid signed 64-bit value '" << s << "'");

  return (int64_t)v;
}


uint128_t String::parseU128(const string &s) {
  int len = s.length();

  if (!startsWith(s, "0x") || len < 3 || 34 < len)
    THROWS("Invalid 128-bit format '" << s << "'");

  uint128_t v;
  int loLen = min(len - 2, 16);
  v.lo = parseU64(string("0x") + s.substr(len - loLen));
  if (18 < len) v.hi = parseU64(s.substr(0, len - loLen));

  return v;
}


double String::parseDouble(const string &s) {
  errno = 0;
  double v = strtod(s.c_str(), 0);
  if (errno) THROWS("Invalid double '" << s << "'");
  return v;
}


float String::parseFloat(const string &s) {
  errno = 0;
  float v = strtof(s.c_str(), 0);
  if (errno) THROWS("Invalid float '" << s << "'");
  return v;
}


bool String::parseBool(const string &s) {
  string v = toLower(trim(s));

  if (v == "true" || v == "t" || v == "1" || v == "yes" || v == "y")
    return true;
  if (v == "false" || v == "f" || v == "0" || v == "no" || v == "n")
    return false;

  THROWS("Invalid bool '" << s << "'");
}


string String::trimLeft(const string &s, const string &delims) {
  string::size_type start = s.find_first_not_of(delims);

  if (start == string::npos) return "";
  return s.substr(start);
}


string String::trimRight(const string &s, const string &delims) {
  string::size_type end = s.find_last_not_of(delims);

  if (end == string::npos) return "";
  return s.substr(0, end + 1);
}


string String::trim(const string &s, const string &delims) {
  string::size_type start = s.find_first_not_of(delims);
  string::size_type end = s.find_last_not_of(delims);

  if (start == string::npos) return "";
  return s.substr(start, (end - start) + 1);
}


string String::toUpper(const string &s) {
  string::size_type len = s.length();
  string v(len, ' ');

  for (string::size_type i = 0; i < len; i++)
    v[i] = toupper(s[i]);

  return v;
}


string String::toLower(const string &s) {
  string::size_type len = s.length();
  string v(len, ' ');

  for (string::size_type i = 0; i < len; i++)
    v[i] = tolower(s[i]);

  return v;
}


string String::capitalize(const string &s) {
  string::size_type len = s.length();
  string v(len, ' ');

  bool whitespace = true;
  for (string::size_type i = 0; i < len; i++) {
    if (whitespace && isalpha(s[i])) v[i] = toupper(s[i]);
    else v[i] = s[i];
    whitespace = isspace(s[i]);
  }

  return v;
}


ostream &String::fill(ostream &stream, const string &str,
                      unsigned currentColumn, unsigned indent,
                      unsigned maxColumn) {
  unsigned pos = currentColumn;
  bool firstWord = true;
  const char *s = str.c_str();

  while (*s) {
    // Skip white space
    while (*s && *s != '\t' && isspace(*s)) {
      if (*s == '\n') {
        stream << '\n';
        pos = 0;
        firstWord = true;
      }

      s++;
    }
    if (!*s) break;

    // Tab in
    while (pos < indent) {
      stream << " ";
      pos++;
    }

    // Get word length
    unsigned len = 1;
    unsigned printLen = 1;
    while (s[len] && (s[len] == '\t' || !isspace(s[len]))) {
      if (s[len] == '\t') printLen++;
      printLen++;
      len++;
    }

    if (!firstWord && pos + printLen + 1 > maxColumn) {
      firstWord = true;
      stream << '\n';
      pos = 0;

    } else {
      if (!firstWord) {
        stream << ' ';
        pos++;
      }
      for (unsigned i = 0; i < len; i++)
        if (s[i] == '\t') stream.write("  ", 2);
        else stream.put(s[i]);

      firstWord = false;
      pos += printLen;
      s += len;
    }
  }

  return stream;
}


string String::fill(const string &str, unsigned currentColumn,
                    unsigned indent, unsigned maxColumn) {
  ostringstream stream;
  fill(stream, str, currentColumn, indent, maxColumn);
  return stream.str();
}


bool String::endsWith(const string &s, const string &part) {
  if (s.length() < part.length()) return false;

  return s.substr(s.length() - part.length()) == part;
}


bool String::startsWith(const string &s, const string &part) {
  if (s.length() < part.length()) return false;

  return s.substr(0, part.length()) == part;
}


string String::bar(const string &title, unsigned width, const string &chars) {
  if (width <= title.length()) return title;

  string result;

  if (title.length()) {
    // Add spaces around title
    result += title + " ";
    if (width <= result.length()) return result;
    result = string(" ") + result;
    if (width <= result.length()) return result;

    unsigned count = (width - result.length()) / 2;

    // Add left part
    string left;
    while (left.length() + chars.length() < count) left += chars;
    if (left.length() < count) left += chars.substr(0, count - left.length());
    result = left + result;
  }

  // Add right part
  while (result.length() + chars.length() < width) result += chars;
  if (result.length() < width)
    result += chars.substr(0, width = result.length());

  return result;
}


string String::hexdump(const char *data, unsigned size) {
  unsigned width = (unsigned)ceil(log((double)size) / log(2.0) / 4);
  string result;
  string chars;
  unsigned i;

  for (i = 0; i < size; i++) {
    if (i % 16 == 0) {
      if (i) {
        result += "  " + chars + '\n';
        chars.clear();
      }
      result += String::printf("0x%0*x", width, i);
    }

    if (i % 16 == 8) {
      result += ' ';
      chars += ' ';
    }

    result += String::printf(" %02x", (unsigned char)data[i]);
    switch (data[i]) {
    case '\a': chars.append("\\a"); break;
    case '\b': chars.append("\\b"); break;
    case '\f': chars.append("\\f"); break;
    case '\n': chars.append("\\n"); break;
    case '\r': chars.append("\\r"); break;
    case '\t': chars.append("\\t"); break;
    case '\v': chars.append("\\v"); break;
    default:
      if (isprint(data[i])) {
        chars.append(1, ' ');
        chars.append(1, data[i]);

      } else chars += " .";
      break;
    }
  }

  // Flush any remaining chars
  if (!chars.empty()) {
    for (; i % 16 != 0; i++) {
      if (i % 16 == 8) result += ' ';
      result.append("   ");
    }

    result += "  " + chars;
  }

  return result;
}


char String::hexNibble(int x, bool lower) {
  x &= 0xf;
  return (x < 0xa ? '0' + x : (lower ? 'a' : 'A') + x - 0xa);
}


string String::hexEncode(const string &s) {
  string result;
  result.reserve(s.length() * 2);

  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    result.append(1, hexNibble(*it >> 4));
    result.append(1, hexNibble(*it));
  }

  return result;
}


string String::escapeRE(const string &s) {
  using namespace boost;
  static const regex esc("[\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\]");
  static const string rep("\\\\\\1&");
  return regex_replace(s, esc, rep, match_default | format_sed);
}


string String::escapeC(const string &s) {
  string result;
  result.reserve(s.length());

  for (string::const_iterator it = s.begin(); it != s.end(); it++)
    switch (*it) {
    case '\"': result.append("\\\""); break;
    case '\\': result.append("\\\\"); break;
    case '\a': result.append("\\a"); break;
    case '\b': result.append("\\b"); break;
    case '\f': result.append("\\f"); break;
    case '\n': result.append("\\n"); break;
    case '\r': result.append("\\r"); break;
    case '\t': result.append("\\t"); break;
    case '\v': result.append("\\v"); break;
    default:
      if (iscntrl(*it)) result.append(printf("\\x%0x", (unsigned)*it));
      else result.push_back(*it);
      break;
    }

  return result;
}


namespace {
  bool is_oct(char c) {
    return '0' <= c && c <= '7';
  }


  bool is_hex(char c) {
    return
      ('a' <= c && c <= 'f') ||
      ('A' <= c && c <= 'F') ||
      ('0' <= c && c <= '9');
  }


  string::const_iterator parseOctalEscape(string &result,
                                          string::const_iterator start,
                                          string::const_iterator end) {
    string::const_iterator it = start + 1;

    string s;
    while (it != end && is_oct(*it) && s.length() < 3) s.push_back(*it++);

    if (s.empty()) return start;

    result.push_back((char)String::parseU8("0" + s));

    return it;
  }


  string::const_iterator parseHexEscape(string &result,
                                        string::const_iterator start,
                                        string::const_iterator end) {
    string::const_iterator it = start + 1;

    string s;
    while (it != end && is_hex(*it) && s.length() < 2) s.push_back(*it++);

    if (s.empty()) return start;

    result.push_back((char)String::parseU8("0x" + s));

    return it;
  }


  string::const_iterator parseUnicodeEscape(string &result,
                                            string::const_iterator start,
                                            string::const_iterator end) {
    string::const_iterator it = start + 1;

    string s;
    while (it != end && is_hex(*it) && s.length() < 4) s.push_back(*it++);

    if (s.length() != 4) return start;

    uint16_t code = String::parseU16("0x" + s);

    if (code < 0x80) result.push_back((char)code);

    else if (code < 0x800) {
      result.push_back(0xc0 | (code >> 6));
      result.push_back(0x80 | (code & 0x3f));

    } else {
      result.push_back(0xe0 | (code >> 12));
      result.push_back(0x80 | ((code >> 6) & 0x3f));
      result.push_back(0x80 | (code & 0x3f));
    }

    return it;
  }
}


string String::unescapeC(const string &s) {
  string result;
  result.reserve(s.length());

  bool escape = false;

  for (string::const_iterator it = s.begin(); it != s.end();) {
    if (escape) {
      escape = false;

      switch (*it) {
      case '0': it = parseOctalEscape(result, it, s.end()); continue;
      case 'a': result.push_back('\a'); break;
      case 'b': result.push_back('\b'); break;
      case 'f': result.push_back('\f'); break;
      case 'n': result.push_back('\n'); break;
      case 'r': result.push_back('\r'); break;
      case 't': result.push_back('\t'); break;
      case 'u': it = parseUnicodeEscape(result, it, s.end()); continue;
      case 'v': result.push_back('\v'); break;
      case 'x': it = parseHexEscape(result, it, s.end()); continue;
      default: result.push_back(*it); break;
      }

    } else if (*it == '\\') escape = true;

    else result.push_back(*it);

    it++;
  }

  return result;
}


string String::join(const vector<string> &s, const string &delim) {
  string result;

  // TODO Could probably be done more efficiently

  for (unsigned i = 0; i < s.size(); i++) {
    if (i) result.append(delim);
    result.append(s[i]);
  }

  return result;
}


size_t String::find(const string &s, const string &pattern,
                    vector<string> *groups) {
  using namespace boost;

  regex e(pattern);
  match_results<string::const_iterator> m;

  if (regex_search(s, m, e)) {
    if (groups)
      for (unsigned i = 0; i < m.size(); i++)
        groups->push_back(string(m[i].first, m[i].second));

    return m.position();
  }

  return string::npos;
}


string String::replace(const string &s, char search, char replace) {
  string result(s);

  for (string::iterator it = result.begin(); it != result.end(); it++)
    if (*it == search) *it = replace;

  return result;
}


string String::replace(const string &s, const string &search,
                       const string &replace) {
  using namespace boost;
  regex exp(search);
  return regex_replace(s, exp, replace, match_default | format_sed);
}


string String::transcode(const string &s, const string &search,
                         const string &replace) {
  if (search.length() != replace.length())
    THROW("Search string must be the same length as the replace string");

  string result(s.length(), ' ');

  unsigned i = 0;
  for (string::const_iterator it = s.begin(); it != s.end(); it++) {
    size_t pos = search.find(*it);
    if (pos != string::npos) result[i++] = replace[pos];
    else result[i++] = *it;
  }

  return result;
}
