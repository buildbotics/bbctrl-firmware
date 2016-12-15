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

#include <cbang/time/Timer.h>

#ifdef _WIN32
#include <cbang/socket/Winsock.h> // For timeval

#include <time.h>
#define WIN32_LEAN_AND_MEAN // Avoid including winsock.h
#include <windows.h>

#else
#include <unistd.h>
#endif

using namespace cb;


Timer::Timer(bool start) : started(false), startTime(0), endTime(0) {
  if (start) this->start();
}


void Timer::start() {
  started = true;
  startTime = now();
}


double Timer::stop() {
  endTime = now();
  started = false;
  return endTime - startTime;
}


double Timer::delta() {
  if (started) return now() - startTime;
  else return endTime - startTime;
}


void Timer::throttle(double seconds) {
  sleep(seconds - delta());
  start();
}


bool Timer::every(double secs) {
  if (!started) {
    start();
    return true;
  }

  double t = now();
  if (secs <= t - startTime) {
    startTime = t;
    return true;
  }

  return false;
}


double Timer::now() {
#ifdef _WIN32
#define EPOCHFILETIME (116444736000000000LL)
  FILETIME ft;
  LARGE_INTEGER li;
  __int64 t;

  GetSystemTimeAsFileTime(&ft);

  li.LowPart = ft.dwLowDateTime;
  li.HighPart = ft.dwHighDateTime;
  t = li.QuadPart;
  t -= EPOCHFILETIME;
  t /= 10;

  return (double)(t / 1000000) + (double)(t % 1000000) * 0.000001;

#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  return toDouble(tv);
#endif
}


double Timer::sleep(double t) {
  if (t <= 0) return 0;

#ifdef _WIN32
  Sleep(1000 * t);

#else
  struct timespec ts = toTimeSpec(t);
  if (nanosleep(&ts, &ts)) return toDouble(ts);
#endif

  return 0;
}


#ifndef _WIN32
struct timespec Timer::toTimeSpec(double t) {
  struct timespec tv;

  tv.tv_sec = (time_t)t;
  tv.tv_nsec = (long)((t - tv.tv_sec) * 1000000000.0);

  return tv;
}


double Timer::toDouble(struct timespec &ts) {
  return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000;
}
#endif // ! _WIN32


struct timeval Timer::toTimeVal(double t) {
  struct timeval tv;

  tv.tv_sec = (time_t)t;
  tv.tv_usec = (long)((t - tv.tv_sec) * 1000000.0);

  return tv;
}


double Timer::toDouble(struct timeval &tv) {
  return (double)tv.tv_sec + (double)tv.tv_usec * 0.000001;
}
