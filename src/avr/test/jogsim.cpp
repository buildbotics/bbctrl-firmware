/******************************************************************************\

    CAMotics is an Open-Source simulation and CAM software.
    Copyright (C) 2011-2017 Joseph Coffland <joseph@cauldrondevelopment.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

\******************************************************************************/

#include "SCurve.h"

#include <iostream>
#include <vector>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

using namespace std;


ostream &operator<<(ostream &stream, const float v[4]) {
  return
    stream << '(' << v[0] << ',' << v[1] << ',' << v[2] << ',' << v[3] << ')';
}


int main(int argc, char *argv[]) {
  if (argc != 6) {
    cout << "Usage: " << argv[0] << " <min soft limit> <max soft limit> "
         << "<max velocity> <max acceleration> <max jerk>" << endl;
    return 1;
  }

  float min = atof(argv[1]);
  float max = atof(argv[2]);
  float maxVel = atof(argv[3]);
  float maxAccel = atof(argv[4]);
  float maxJerk = atof(argv[5]);

  vector<SCurve> scurves;
  for (unsigned axis = 0; axis < 4; axis++)
    scurves.push_back(SCurve(maxVel, maxAccel, maxJerk));

  float p[4];
  float targetV[4];
  float lastP[4];
  const float deltaT = 0.005 / 60;
  const float coastV = 10; // mm/min

  const size_t maxLen = 1024;
  char *line = new char[maxLen];

  // Non-blocking input
  fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

  while (true) {
    while (true) {
      struct timeval tv = {0, 0};
      fd_set fds;

      FD_ZERO(&fds);
      FD_SET(0, &fds);

      int ret = select(1, &fds, 0, 0, &tv);
      if (ret == -1) return 0;
      if (!ret) break;

      ssize_t len = getline(&line, (size_t *)&maxLen, stdin);
      if (len < 0) break;

      const char *delims = ", \t\n\r";
      char *s = line;

      for (unsigned i = 0; i < 4; i++) {
        char *token = strtok(s, delims);
        if (!token) break;
        targetV[i] = atof(token) * maxVel;
        s = 0;
      }
    }

    float v[4], a[4], j[4], d[4];
    bool changed = false;

    for (unsigned axis = 0; axis < 4; axis++) {
      float vel = scurves[axis].getVelocity();
      float targetVel = targetV[axis];

      if (coastV < fabs(targetVel)) {
        float dist = scurves[axis].getStoppingDist() * 1.01;

        if (vel < 0 && p[axis] - dist <= min) targetVel = -coastV;
        if (0 < vel && max <= p[axis] + dist) targetVel = coastV;
      }

      v[axis] = scurves[axis].next(deltaT, targetVel);
      a[axis] = scurves[axis].getAcceleration();
      j[axis] = scurves[axis].getJerk();
      d[axis] = scurves[axis].getStoppingDist();

      float deltaP = v[axis] * deltaT;
      if (0 < deltaP && max < p[axis] + deltaP) p[axis] = max;
      else if (deltaP < 0 && p[axis] + deltaP < min) p[axis] = min;
      else p[axis] += deltaP;

      if (lastP[axis] != p[axis]) {
        changed = true;
        lastP[axis] = p[axis];
      }
    }

    if (changed)
      cout << p << ' ' << targetV << ' ' << v << ' ' << a << ' ' << j << ' '
           << d << endl;

    usleep(5000); // 5ms
  }
}
