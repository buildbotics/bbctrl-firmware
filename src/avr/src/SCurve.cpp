/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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


#include "SCurve.h"

#include <math.h>


SCurve::SCurve(float maxV, float maxA, float maxJ) :
  maxV(maxV), maxA(maxA), maxJ(maxJ), v(0), a(0), j(0) {}


unsigned SCurve::getPhase() const {
  if (!v) return 0;

  // Handle negative velocity
  float v = this->v;
  float a = this->a;
  if (v < 0) {
    v = -v;
    a = -a;
  }

  if (0 < a) {
    if (0 < j) return 1;
    if (!j) return 2;
    return 3;
  }

  if (!a) return 4;
  if (j < 0) return 5;
  if (!j) return 6;

  return 7;
}


float SCurve::getStoppingDist() const {return stoppingDist(v, a, maxA, maxJ);}


float SCurve::next(float t, float targetV, float overrideJ) {
  // Compute next acceleration
  float nextA =
    nextAccel(t, targetV, v, a, maxA, isfinite(overrideJ) ? overrideJ : maxJ);

  // Compute next velocity
  float deltaV = nextA * t;
  if ((deltaV < 0 && targetV < v && v + deltaV < targetV) ||
      (0 < deltaV && v < targetV && targetV < v + deltaV)) {
    nextA = (targetV - v) / t;
    v = targetV;

  } else v += deltaV;

  // Compute jerk = delta accel / time
  j = (nextA - a) / t;
  a = nextA;

  return v;
}


float SCurve::adjustedJerkForStoppingDist(float d) const {
  // Only make adjustments in phase 7
  if (getPhase() != 7) return NAN;

  // Handle negative velocity
  float v = this->v;
  float a = this->a;
  if (v < 0) {
    v = -v;
    a = -a;
  }

  // Compute distance to zero vel
  float actualD = distance(-a / maxJ, v, 0, maxJ);
  const float fudge = 0.05;
  if (actualD < d) return maxJ * (1 + fudge);
  else if (d < actualD) return maxJ * (1 - fudge);
  return maxJ;

  // Compute jerk to hit distance exactly
  //
  // S-curve distance formula
  // d = vt + 1/2 at^2 + 1/6 jt^3
  //
  // t = -a/j when a < 0
  //
  // Substitute and simplify
  // (d = -va/j + (a^3)/(2j^2) - (a^3)/(6j^2)) * j^2
  // dj^2 = -jva + 1/2 a^3 - 1/6 a^3
  // dj^2 + vaj - 1/3 a^3 = 0
  //
  // Apply quadratic formula
  // j = (-va - sqrt((va)^2 + 4/3 da^3)) / (2d)
  return -a * (v + sqrt(v * v + 1.333333 * d * a)) / (2 * d);
}


float SCurve::stoppingDist(float v, float a, float maxA, float maxJ) {
  // Already stopped
  if (!v) return 0;

  // Handle negative velocity
  if (v < 0) {
    v = -v;
    a = -a;
  }

  float d = 0;

  // Compute distance and velocity change to accel = 0
  if (0 < a) {
    // Compute distance to decrease accel to zero
    float t = a / maxJ;
    d += distance(t, v, a, -maxJ);
    v += velocity(t, a, -maxJ);

  } else if (a < 0) {
    // Compute distance to increase accel to zero
    float t = a / -maxJ;
    d -= distance(t, v, a, maxJ);
    v -= velocity(t, a, maxJ);
  }

  // Compute max deccel from zero accel
  float maxDeccel = -sqrt(v * maxJ);
  if (maxDeccel < -maxA) maxDeccel = -maxA;

  // Compute distance and velocity change to max deccel
  float t = maxDeccel / -maxJ;
  d += distance(t, v, 0, -maxJ);
  float deltaV = velocity(t, 0, -maxJ);
  v += deltaV;

  // Compute constant deccel period
  if (-deltaV < v) {
    float t = (v + deltaV) / -maxDeccel;
    d += distance(t, v, maxDeccel, 0);
    v += velocity(t, maxDeccel, 0);
  }

  // Compute distance to zero vel
  d += distance(t, v, maxDeccel, maxJ);

  return d;
}


float SCurve::nextAccel(float t, float targetV, float v, float a, float maxA,
                        float maxJ) {
  bool increasing = v < targetV;
  float deltaA = acceleration(t, maxJ);

  if (increasing && a < -deltaA)
    return a + deltaA; // negative accel, increasing speed

  if (!increasing && deltaA < a)
    return a - deltaA; // positive accel, decreasing speed

  float deltaV = fabs(targetV - v);
  float targetA = sqrt(2 * deltaV * maxJ);
  if (maxA < targetA) targetA = maxA;

  if (increasing) {
    if (targetA < a + deltaA) return targetA;
    return a + deltaA;

  } else {
    if (a - deltaA < -targetA) return -targetA;
    return a - deltaA;
  }
}


float SCurve::distance(float t, float v, float a, float j) {
  // v * t + 1/2 * a * t^2 + 1/6 * j * t^3
  return t * (v + t * (0.5 * a + 1.0 / 6.0 * j * t));
}


float SCurve::velocity(float t, float a, float j) {
  // a * t + 1/2 * j * t^2
  return t * (a + 0.5 * j * t);
}


float SCurve::acceleration(float t, float j) {return j * t;}
