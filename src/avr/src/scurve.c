/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                            All rights reserved.

     This file ("the software") is free software: you can redistribute it
     and/or modify it under the terms of the GNU General Public License,
      version 2 as published by the Free Software Foundation. You should
      have received a copy of the GNU General Public License, version 2
     along with the software. If not, see <http://www.gnu.org/licenses/>.

     The software is distributed in the hope that it will be useful, but
          WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
               Lesser General Public License for more details.

       You should have received a copy of the GNU Lesser General Public
                License along with the software.  If not, see
                       <http://www.gnu.org/licenses/>.

                For information regarding this software email:
                  "Joseph Coffland" <joseph@buildbotics.com>

\******************************************************************************/

#include "scurve.h"

#include <math.h>


float scurve_distance(float t, float v, float a, float j) {
  // v * t + 1/2 * a * t^2 + 1/6 * j * t^3
  return t * (v + t * (0.5 * a + 1.0 / 6.0 * j * t));
}


float scurve_velocity(float t, float a, float j) {
  // a * t + 1/2 * j * t^2
  return t * (a + 0.5 * j * t);
}


float scurve_acceleration(float t, float j) {return j * t;}


float scurve_next_accel(float dT, float iV, float tV, float iA, float jerk) {
  float tA = sqrt(jerk * fabs(tV - iV)) * (tV < iV ? -1 : 1); // Target accel
  float dA = jerk * dT; // Delta accel

  if (iA < tA) return (iA < tA + dA) ? tA : (iA + dA);
  if (tA < iA) return (iA - dA < tA) ? tA : (iA - dA);

  return iA;
}
