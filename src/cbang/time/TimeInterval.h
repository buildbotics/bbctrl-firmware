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

#ifndef CBANG_TIME_INTERVAL_H
#define CBANG_TIME_INTERVAL_H

#include <cbang/StdTypes.h>

#include <ostream>
#include <string>

namespace cb {
  /**
   * Used for printing time intervals in a common format.
   *
   * E.g.  cout << TimeInterval(interval) << endl;
   */
  class TimeInterval {
    double interval;
    bool compact;

  public:
    /// @param interval In seconds.
    TimeInterval(double interval, bool compact = false) :
      interval(interval), compact(compact) {}

    std::string toString() const;
  };

  inline std::ostream &operator<<(std::ostream &stream, const TimeInterval &t) {
    return stream << t.toString();
  }
}

#endif // CBANG_TIME_INTERVAL_H

