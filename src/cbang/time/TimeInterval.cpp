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

#include <cbang/time/TimeInterval.h>

#include <cbang/Exception.h>
#include <cbang/String.h>

#include <cbang/time/Time.h>

#include <cbang/os/SystemUtilities.h>

#include <iomanip>

using namespace std;
using namespace cb;


string TimeInterval::toString() const {
  double i = interval < 0 ? -interval : interval;

  if (Time::SEC_PER_YEAR < i) {
    return String::printf("%0.2f%s", interval / Time::SEC_PER_YEAR,
                          compact ? "y" : " years");

  } else if (Time::SEC_PER_DAY < i) {
    return String::printf("%0.2f%s", interval / Time::SEC_PER_DAY,
                          compact ? "d" : " days");

  } else if (Time::SEC_PER_HOUR < i) {
    int hours = (int)interval / Time::SEC_PER_HOUR;
    int mins = ((int)interval % Time::SEC_PER_HOUR) / Time::SEC_PER_MIN;

    return String::printf("%d%s %02d%s", hours, compact ? "h" : " hours", mins,
                          compact ? "m" : " mins");

  } else if (Time::SEC_PER_MIN < i) {
    int mins = (int)interval / Time::SEC_PER_MIN;
    int secs = (int)interval % Time::SEC_PER_MIN;

    return String::printf("%d%s %02d%s", mins, compact ? "m" : " mins", secs,
                          compact ? "s" : " secs");
  }

  return String::printf("%0.2f%s", interval, compact ? "s" : " secs");
}
