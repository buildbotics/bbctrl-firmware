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

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/time/Time.h>

#include <sstream>
#include <locale>
#include <exception>

#include <boost/date_time/posix_time/posix_time.hpp>

using namespace std;
using namespace cb;

using namespace boost::posix_time;
using namespace boost::gregorian;

const unsigned Time::SEC_PER_MIN  = 60;
const unsigned Time::SEC_PER_HOUR = Time::SEC_PER_MIN  * 60;
const unsigned Time::SEC_PER_DAY  = Time::SEC_PER_HOUR * 24;
const unsigned Time::SEC_PER_YEAR = Time::SEC_PER_DAY  * 365;

namespace {
  const date epoch(1970, 1, 1);
}


const char *Time::defaultFormat = "%Y-%m-%dT%H:%M:%SZ"; // ISO 8601


Time::Time(uint64_t time, const string &format) :
  format(format), time(time == ~(uint64_t)0 ? now() : time) {
}


Time::Time(const string &format) : format(format), time(now()) {}


string Time::toString() const {
  if (!time) return "<invalid>";

  try {
    time_facet *facet = new time_facet();
    facet->format(format.c_str());

    ptime t(epoch, seconds(time));
    stringstream ss;
    ss.imbue(locale(ss.getloc(), facet));
    ss << t;

    return ss.str();

  } catch (const exception &e) {
    THROWS("Failed to format time '" << time << "' with format '" << format
           << "': " << e.what());
  }
}


Time Time::parse(const string &s, const string &format) {
  try {
    time_input_facet *facet = new time_input_facet();
    facet->format(format.c_str());

    ptime t;
    stringstream ss(s);
    ss.imbue(locale(ss.getloc(), facet));
    ss >> t;

    time_duration diff = t - ptime(epoch);

    return Time(diff.total_seconds(), format);

  } catch (const exception &e) {
    THROWS("Failed to parse time '" << s << "' with format '" << format
           << "': " << e.what());
  }
}


uint64_t Time::now() {
  return (uint64_t)(second_clock::universal_time() -
                    ptime(epoch)).total_seconds();
}


int32_t Time::offset() {
  return (int32_t)(second_clock::local_time() -
                   second_clock::universal_time()).total_seconds();
}
