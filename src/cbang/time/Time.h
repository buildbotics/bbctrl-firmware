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

#ifndef CBANG_TIME_H
#define CBANG_TIME_H

#include <cbang/StdTypes.h>
#include <ostream>
#include <string>

namespace cb {
  /**
   * Used for printing and parsing times.  Should be 64-bit everywhere and
   * there for safe past year 2038.
   *
   * E.g.  cout << Time(Time::now() + Time::SEC_PER_HOUR, "%H:%M:%S") << endl;
   */
  class Time {
    std::string format;
    uint64_t time;

  public:
    static const char *defaultFormat;

    static const unsigned SEC_PER_MIN;
    static const unsigned SEC_PER_HOUR;
    static const unsigned SEC_PER_DAY;
    static const unsigned SEC_PER_YEAR;

    /// @param time In seconds since Janary 1st, 1970
    Time(uint64_t time = ~(uint64_t)0,
         const std::string &format = defaultFormat);
    Time(const std::string &format);

    std::string toString() const;
    operator std::string () const {return toString();}
    operator uint64_t () const {return time;}

    bool operator==(const Time &o) const {return time == o.time;}
    bool operator!=(const Time &o) const {return time != o.time;}
    bool operator< (const Time &o) const {return time <  o.time;}
    bool operator<=(const Time &o) const {return time <= o.time;}
    bool operator> (const Time &o) const {return time >  o.time;}
    bool operator>=(const Time &o) const {return time >= o.time;}

    static Time parse(const std::string &s,
                      const std::string &format = defaultFormat);

    /// Get current time in seconds since Janary 1st, 1970.
    static uint64_t now();

    // UTC offset in seconds
    static int32_t offset();
 };

  inline std::ostream &operator<<(std::ostream &stream, const Time &t) {
    return stream << t.toString();
  }
}

#endif // CBANG_TIME_H
