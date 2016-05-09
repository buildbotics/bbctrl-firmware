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

#ifndef CBANG_TIMER_H
#define CBANG_TIMER_H

#ifdef _MSC_VER
struct timeval {
  long tv_sec;
  long tv_usec;
};

#undef max
#undef min

#else // _WIN32
#include <time.h>
#include <sys/time.h>
#endif // _WIN32

namespace cb {
  /// Time events or access current system time
  class Timer {
    bool started;
    double startTime;
    double endTime;

  public:
    /**
     * Create a new timer
     *
     * @param start If true start the timer at construction.
     */
    Timer(bool start = false);

    bool isRunning() const {return started;}

    /// Start or restart timing
    void start();

    /**
     * Stop timer
     * @return Time since last call to start().
     */
    double stop();

    /// @return The difference in time since the last call to start() and now
    double delta();

    /// Delay so that calls to this function happen no more than @param seconds
    void throttle(double seconds);

    /**
     * Can be used to do something on a regular interval.
     *
     * @param secs How often this function should return true.
     *
     * @return True if the event should occur now, false if it should wait.
     */
    bool every(double secs);

    /**
     * Fractional part indicates fractions of a second.
     *
     * @return The curent system time in seconds since 1970.
     */
    static double now();

    static double sleep(double t);

#ifndef _WIN32
    static struct timespec toTimeSpec(double t);
    static double toDouble(struct timespec &t);
#endif // _WIN32
    static struct timeval toTimeVal(double t);
    static double toDouble(struct timeval &t);
  };
}

#endif // CBANG_TIMER_H
