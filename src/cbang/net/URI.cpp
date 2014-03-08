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

#include "URI.h"

#include <cbang/String.h>
#include <cbang/Exception.h>

#include <boost/version.hpp>

#if (BOOST_VERSION < 103600)
#include <boost/spirit/core.hpp>
#include <boost/spirit/actor/assign_actor.hpp>
#include <boost/spirit/actor/insert_at_actor.hpp>
#include <boost/spirit/actor/clear_actor.hpp>

using namespace boost::spirit;

#else
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_assign_actor.hpp>
#include <boost/spirit/include/classic_insert_at_actor.hpp>
#include <boost/spirit/include/classic_clear_actor.hpp>

using namespace boost::spirit::classic;
#endif

#include <sstream>

#include <ctype.h>
#include <string.h>

using namespace cb;
using namespace std;

typedef rule<scanner<string::const_iterator> > rule_t;

static const rule_t reserved =
  ch_p(';') | '/' | '?' | ':' | '@' | '&' | '=' | '+' | '$' | ',';

static const rule_t unreserved =
  alnum_p | '-' | '_' | '.' | '!' | '~' | '*' | '\'' | '(' | ')';

static const rule_t escaped = ch_p('%') >> xdigit_p >> xdigit_p;

static const rule_t uric = reserved | unreserved | escaped;

static const rule_t user_pass =
  *(unreserved | escaped | ';' | '&' | '=' | '+' | '$' | ',');

static const rule_t name_char =
  unreserved | escaped | ';' | '/' | '?' | ':' | '@' | '+' | '$' | ',';

static const rule_t value_char =
  name_char | '=';

static const rule_t segment =
  *(unreserved | escaped | ':' | '@' | '&' | '=' | '+' | '$' | ',');

static const rule_t path_segments = segment >> *(ch_p('/') >> segment);


URI::URI() : port(0) {
}


URI::URI(const string &uri) : port(0) {
  read(uri);
}


URI::URI(const char *uri) : port(0) {
  read(uri);
}


unsigned URI::getPort() const {
  if (port || scheme.empty()) return port;

  if (scheme == "ftp") return 21;
  if (scheme == "ssh") return 22;
  if (scheme == "telnet") return 23;
  if (scheme == "domain") return 53;
  if (scheme == "tftp") return 69;
  if (scheme == "gopher") return 70;
  if (scheme == "finger") return 79;
  if (scheme == "http") return 80;
  if (scheme == "pop2") return 109;
  if (scheme == "pop3") return 110;
  if (scheme == "auth") return 113;
  if (scheme == "sftp") return 115;
  if (scheme == "nntp") return 119;
  if (scheme == "ntp") return 123;
  if (scheme == "snmp") return 161;
  if (scheme == "irc") return 194;
  if (scheme == "imap3") return 220;
  if (scheme == "ldap") return 389;
  if (scheme == "https") return 443;

  THROWS("Unknown scheme '" << scheme << "' and port not set");
}


const string URI::getQuery() const {
  ostringstream str;
  writeQuery(str);
  return decode(str.str());
}


namespace {
  struct assign_decoded {
    string &target;

    assign_decoded(string &target) : target(target) {}

    void operator()(string::const_iterator first,
                    string::const_iterator last) const {
      target = URI::decode(string(first, last));
    }
  };
}


void URI::setQuery(const string &query) {
  string key;
  string value;

  clear(); // Reset query string vars

  rule_t pair =
    ((+name_char)[assign_decoded(key)][clear_a(value)] >>
     !(ch_p('=') >> (*value_char)[assign_decoded(value)]))
    [insert_at_a(*this, key, value)];

  rule_t parser = pair >> *(ch_p('&') >> pair);

  parse_info<string::const_iterator> info =
    parse(query.begin(), query.end(), parser);

  if (!info.full) THROWS("Invalid URI query '" << query << "'");
}


void URI::normalize() {
  // TODO: Resolove '..' and '.'

  vector<string> parts;
  String::tokenize(path, parts, "/");
  path = string("/") + String::join(parts, "/");
}


void URI::read(const string &uri) {
  // See RFC2396
  // Does not support:
  //   opaque URI data (Section 3)
  //   registry-based naming (Section 3.2.1)
  //   path parameters (Section 3.3)

  clear(); // Reset query string vars

  rule_t scheme =
    (alpha_p >> *(alnum_p | '+' | '-' | '.'))[assign_a(this->scheme)] >> ':';

  bool haveUserInfo = false;
  rule_t userinfo =
    user_pass[assign_a(user)] >> !(':' >> user_pass[assign_a(pass)]);

  rule_t host = +(alnum_p | '-' | '.');

  rule_t hostport =
    host[assign_a(this->host)] >> !(':' >> !(uint_p[assign_a(this->port)]));

  rule_t authority =
    !(userinfo >> '@')[assign_a(haveUserInfo, true)] >> hostport;

  rule_t abs_path = (ch_p('/') >> path_segments)[assign_a(path)];

  rule_t net_path = (strlit<>("//") | ch_p('/')) >> authority >> !abs_path;

  string queryStr;
  rule_t query = ch_p('?') >> (*uric)[assign_a(queryStr)];

  rule_t parser = ((scheme >> net_path) | abs_path) >> !query;

  parse_info<string::const_iterator> info =
    parse(uri.begin(), uri.end(), parser);

  if (!info.full) THROWS("Invalid URI '" << uri << "'");

  if (!haveUserInfo) {
    setUser("");
    setPass("");
  }
  if (!queryStr.empty()) setQuery(queryStr);
}


ostream &URI::write(ostream &stream) const {
  if (!scheme.empty()) stream << scheme << ':';
  if (!host.empty()) {
    stream << "//";

    if (!user.empty()) stream << encode(user);
    if (!pass.empty()) stream << ':' << encode(pass);
    if (!user.empty() || !pass.empty()) stream << '@';

    stream << encode(host);
    if (port) stream << ':' << port;
  }

  stream << encode(path);

  if (!empty()) stream << '?';

  return writeQuery(stream);
}


ostream &URI::writeQuery(ostream &stream) const {
  for (const_iterator it = begin(); it != end(); it++) {
    if (it != begin()) stream << '&';

    stream << encode(it->first);
    if (!it->second.empty()) stream << '=' << encode(it->second);
  }

  return stream;
}


string URI::toString() const {
  ostringstream str;
  str << *this;
  return str.str();
}


string URI::encode(const string &s) {
  const char *not_escaped = "-_.!~*'()/";
  ostringstream str;

  for (unsigned i = 0; i < s.length(); i++)
    if (isalnum(s[i]) || strchr(not_escaped, s[i])) str << s[i];
    else str << String::printf("%%%02x", (unsigned)s[i]);

  return str.str();
}


int URI::hex2digit(char x) {
  return isdigit(x) ? x - '0' : ((islower(x) ? x - 'a' : x - 'A') + 10);
}


string URI::decode(const string &s) {
  string result(s.length(), ' ');
  unsigned len = 0;

  for (unsigned i = 0; i < s.length(); i++)
    if (s[i] == '%' && i < s.length() - 2 &&
        isxdigit(s[i + 1]) && isxdigit(s[i + 2])) {
      result[len++] = (char)((hex2digit(s[i + 1]) << 4) + hex2digit(s[i + 2]));
      i += 2;

    } else result[len++] = s[i];

  result.resize(len);

  return result;
}
