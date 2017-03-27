/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2012 - 2015 Rob Giseburt
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

#include "velocity_curve.h"


/// We are using a quintic (fifth-degree) Bezier polynomial for the velocity
/// curve.  This yields a constant pop; with pop being the sixth derivative
/// of position:
///
///   1st - velocity
///   2nd - acceleration
///   3rd - jerk
///   4th - snap
///   5th - crackle
///   6th - pop
///
/// The Bezier curve takes the form:
///
///   f(t) = P_0(1 - t)^5 + 5P_1(1 - t)^4 t + 10P_2(1 - t)^3 t^2 +
///          10P_3(1 - t)^2 t^3 + 5P_4(1 - t)t^4 + P_5t^5
///
/// Where 0 <= t <= 1, f(t) is the velocity and P_0 through P_5 are the control
/// points.  In our case:
///
///   P_0 = P_1 = P2 = Vi
///   P_3 = P_4 = P5 = Vt
///
/// Where Vi is the initial velocity and Vt is the target velocity.
///
/// After substitution, expanding the polynomial and collecting terms we have:
///
///    f(t) = (Vt - Vi)(6t^5 - 15t^4 + 10t^3) + Vi
///
/// Computing this directly using 32bit float-point on a 32MHz AtXMega processor
/// takes about 60uS or about 1,920 clocks.  The code was compiled with avr-gcc
/// v4.9.2 with -O3.
float velocity_curve(float Vi, float Vt, float t) {
  const float t2 = t * t;
  const float t3 = t2 * t;

  return (Vt - Vi) * (6 * t2 - 15 * t + 10) * t3 + Vi;
}
