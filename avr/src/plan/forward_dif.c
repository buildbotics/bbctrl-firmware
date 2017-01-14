/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include "forward_dif.h"

#include <math.h>


/// Forward differencing math
///
/// We are using a quintic (fifth-degree) Bezier polynomial for the velocity
/// curve.  This gives us a "linear pop" velocity curve; with pop being the
/// sixth derivative of position: velocity - 1st, acceleration - 2nd, jerk -
/// 3rd, snap - 4th, crackle - 5th, pop - 6th
///
/// The Bezier curve takes the form:
///
///   V(t) = P_0 * B_0(t) + P_1 * B_1(t) + P_2 * B_2(t) + P_3 * B_3(t) +
///          P_4 * B_4(t) + P_5 * B_5(t)
///
/// Where 0 <= t <= 1, and V(t) is the velocity. P_0 through P_5 are
/// the control points, and B_0(t) through B_5(t) are the Bernstein
/// basis as follows:
///
///   B_0(t) =   (1 - t)^5        =   -t^5 +  5t^4 - 10t^3 + 10t^2 -  5t + 1
///   B_1(t) =  5(1 - t)^4 * t    =   5t^5 - 20t^4 + 30t^3 - 20t^2 +  5t
///   B_2(t) = 10(1 - t)^3 * t^2  = -10t^5 + 30t^4 - 30t^3 + 10t^2
///   B_3(t) = 10(1 - t)^2 * t^3  =  10t^5 - 20t^4 + 10t^3
///   B_4(t) =  5(1 - t)   * t^4  =  -5t^5 +  5t^4
///   B_5(t) =               t^5  =    t^5
///
///                                      ^       ^       ^       ^     ^   ^
///                                      A       B       C       D     E   F
///
/// We use forward-differencing to calculate each position through the curve.
/// This requires a formula of the form:
///
///   V_f(t) = A * t^5 + B * t^4 + C * t^3 + D * t^2 + E * t + F
///
/// Looking at the above B_0(t) through B_5(t) expanded forms, if we take the
/// coefficients of t^5 through t of the Bezier form of V(t), we can determine
/// that:
///
///   A =      -P_0 +  5 * P_1 - 10 * P_2 + 10 * P_3 -  5 * P_4 +  P_5
///   B =   5 * P_0 - 20 * P_1 + 30 * P_2 - 20 * P_3 +  5 * P_4
///   C = -10 * P_0 + 30 * P_1 - 30 * P_2 + 10 * P_3
///   D =  10 * P_0 - 20 * P_1 + 10 * P_2
///   E = - 5 * P_0 +  5 * P_1
///   F =       P_0
///
/// Now, since we will (currently) *always* want the initial acceleration and
/// jerk values to be 0, We set P_i = P_0 = P_1 = P_2 (initial velocity), and
/// P_t = P_3 = P_4 = P_5 (target velocity), which, after simplification,
/// resolves to:
///
///   A = - 6 * P_i +  6 * P_t
///   B =  15 * P_i - 15 * P_t
///   C = -10 * P_i + 10 * P_t
///   D = 0
///   E = 0
///   F = P_i
///
/// Given an interval count of I to get from P_i to P_t, we get the parametric
/// "step" size of h = 1/I.  We need to calculate the initial value of forward
/// differences (F_0 - F_5) such that the inital velocity V = P_i, then we
/// iterate over the following I times:
///
///   V   += F_5
///   F_5 += F_4
///   F_4 += F_3
///   F_3 += F_2
///   F_2 += F_1
///
/// See
/// http://www.drdobbs.com/forward-difference-calculation-of-bezier/184403417
/// for an example of how to calculate F_0 - F_5 for a cubic bezier curve. Since
/// this is a quintic bezier curve, we need to extend the formulas somewhat.
/// I'll not go into the long-winded step-by-step here, but it gives the
/// resulting formulas:
///
///   a = A, b = B, c = C, d = D, e = E, f = F
///
///   F_5(t + h) - F_5(t) = (5ah)t^4 + (10ah^2 + 4bh)t^3 +
///     (10ah^3 + 6bh^2 + 3ch)t^2 + (5ah^4 + 4bh^3 + 3ch^2 + 2dh)t + ah^5 +
///     bh^4 + ch^3 + dh^2 + eh
///
///   a = 5ah
///   b = 10ah^2 + 4bh
///   c = 10ah^3 + 6bh^2 + 3ch
///   d = 5ah^4 + 4bh^3 + 3ch^2 + 2dh
///
/// After substitution, simplification, and rearranging:
///
///   F_4(t + h) - F_4(t) = (20ah^2)t^3 + (60ah^3 + 12bh^2)t^2 +
///     (70ah^4 + 24bh^3 + 6ch^2)t + 30ah^5 + 14bh^4 + 6ch^3 + 2dh^2
///
///   a = 20ah^2
///   b = 60ah^3 + 12bh^2
///   c = 70ah^4 + 24bh^3 + 6ch^2
///
/// After substitution, simplification, and rearranging:
///
///   F_3(t + h) - F_3(t) = (60ah^3)t^2 + (180ah^4 + 24bh^3)t + 150ah^5 +
///     36bh^4 + 6ch^3
///
/// You get the picture...
///
///   F_2(t + h) - F_2(t) = (120ah^4)t + 240ah^5 + 24bh^4
///   F_1(t + h) - F_1(t) = 120ah^5
///
/// Normally, we could then assign t = 0, use the A-F values from above, and get
/// out initial F_* values.  However, for the sake of "averaging" the velocity
/// of each segment, we actually want to have the initial V be be at t = h/2 and
/// iterate I-1 times.  So, the resulting F_* values are (steps not shown):
///
///   F_5 = 121Ah^5 / 16 + 5Bh^4 + 13Ch^3 / 4 + 2Dh^2 + Eh
///   F_4 = 165Ah^5 / 2 + 29Bh^4 + 9Ch^3 + 2Dh^2
///   F_3 = 255Ah^5 + 48Bh^4 + 6Ch^3
///   F_2 = 300Ah^5 + 24Bh^4
///   F_1 = 120Ah^5
///
/// Note that with our current control points, D and E are actually 0.
///
/// This can be simplified even further by subsituting Ah, Bh & Ch back in and
/// reducing to:
///
///   F_5 = (32.5 * s^2 -  75 * s +   45.375)(Vt - Vi) * h^5
///   F_4 = (90.0 * s^2 - 435 * s +  495.0  )(Vt - Vi) * h^5
///   F_3 = (60.0 * s^2 - 720 * s + 1530.0  )(Vt - Vi) * h^5
///   F_2 = (           - 360 * s + 1800.0  )(Vt - Vi) * h^5
///   F_1 = (                        720.0  )(Vt - Vi) * h^5
///
float mp_init_forward_dif(forward_dif_t fdif, float Vi, float Vt, float s) {
  const float h = 1 / s;
  const float s2 = square(s);
  const float Vdxh5 = (Vt - Vi) * h * h * h * h * h;

  fdif[4] = (32.5 * s2 -  75.0 * s +   45.375) * Vdxh5;
  fdif[3] = (90.0 * s2 - 435.0 * s +  495.0  ) * Vdxh5;
  fdif[2] = (60.0 * s2 - 720.0 * s + 1530.0  ) * Vdxh5;
  fdif[1] = (          - 360.0 * s + 1800.0  ) * Vdxh5;
  fdif[0] = (                         720.0  ) * Vdxh5;

  // Calculate the initial velocity by calculating:
  //
  //   V(h / 2) =
  //
  //   ( -6Vi +  6Vt) / (2^5 * s^8)  +
  //   ( 15Vi - 15Vt) / (2^4 * s^8) +
  //   (-10Vi + 10Vt) / (2^3 * s^8) + Vi =
  //
  //     (Vt - Vi) * 1/2 * h^8 + Vi
  return (Vt - Vi) * 0.5 * square(square(square(h))) + Vi;
}


float mp_next_forward_dif(forward_dif_t fdif) {
  float delta = fdif[4];

  for (int i = 4; i; i--)
    fdif[i] += fdif[i - 1];

  return delta;
}
