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

#include "line.h"

#include "axis.h"
#include "planner.h"
#include "exec.h"
#include "buffer.h"

#include <stdio.h>


/* Sonny's algorithm - simple
 *
 * Computes the maximum allowable junction speed by finding the velocity that
 * will yield the centripetal acceleration in the corner_acceleration value. The
 * value of delta sets the effective radius of curvature. Here's Sonny's
 * (Sungeun K. Jeon's) explanation of what's going on:
 *
 * "First let's assume that at a junction we only look a centripetal
 * acceleration to simplify things.  At a junction of two lines, let's place a
 * circle such that both lines are tangent to the circle.  The circular segment
 * joining the lines represents the path for constant centripetal acceleration.
 * This creates a deviation from the path (let's call this delta),
 * which is the distance from the junction to the edge of the circular
 * segment.  Delta needs to be defined, so let's replace the term max_jerk (see
 * note 1) with max junction deviation, or "delta".  This indirectly sets the
 * radius of the circle, and hence limits the velocity by the centripetal
 * acceleration.  Think of the this as widening the race track. If a race car is
 * driving on a track only as wide as a car, it'll have to slow down a lot to
 * turn corners.  If we widen the track a bit, the car can start to use the
 * track to go into the turn.  The wider it is, the faster through the corner
 * it can go.
 *
 * Note 1: "max_jerk" refers to the old grbl/marlin "max_jerk" approximation
 * term, not the TinyG jerk terms.
 *
 * If you do the geometry in terms of the known variables, you get:
 *
 *     sin(theta/2) = R / (R + delta)
 *
 * Re-arranging in terms of circle radius (R)
 *
 *     R = delta * sin(theta/2) / (1 - sin(theta/2))
 *
 * Theta is the angle between line segments given by:
 *
 *     cos(theta) = dot(a, b) / (norm(a) * norm(b))
 *
 * Most of these calculations are already done in the planner.
 * To remove the acos() and sin() computations, use the trig half
 * angle identity:
 *
 *     sin(theta/2) = +/-sqrt((1 - cos(theta)) / 2)
 *
 * For our applications, this should always be positive.  Now just plug the
 * equations into the centripetal acceleration equation:
 *
 *    v_c = sqrt(a_max * R)
 *
 * You'll see that there are only two sqrt computations and no sine/cosines.
 *
 * How to compute the radius using brute-force trig:
 *
 *    float theta = acos(costheta);
 *    float radius = delta * sin(theta/2) / (1 - sin(theta/2));
 *
 * This version extends Chamnit's algorithm by computing a value for delta that
 * takes the contributions of the individual axes in the move into account.
 * This allows the control radius to vary by axis.  This is necessary to
 * support axes that have different dynamics; such as a Z axis that doesn't
 * move as fast as X and Y (such as a screw driven Z axis on machine with a belt
 * driven XY - like a Shapeoko), or rotary axes ABC that have completely
 * different dynamics than their linear counterparts.
 *
 * The function takes the absolute values of the sum of the unit vector
 * components as a measure of contribution to the move, then scales the delta
 * values from the non-zero axes into a composite delta to be used for the
 * move.  Shown for an XY vector:
 *
 *     U[i]    Unit sum of i'th axis    fabs(unit_a[i]) + fabs(unit_b[i])
 *     Usum    Length of sums           Ux + Uy
 *     d       Delta of sums            (Dx * Ux + DY * UY) / Usum
 */
static float _get_junction_vmax(const float a_unit[], const float b_unit[]) {
  float costheta = 0;
  for (int axis = 0; axis < AXES; axis++)
    costheta -= a_unit[axis] * b_unit[axis];

  if (costheta < -0.99) return 10000000;  // straight line cases
  if (0.99 < costheta)  return 0;         // reversal cases

  // Fuse the junction deviations into a vector sum
  float a_delta = 0;
  float b_delta = 0;

  for (int axis = 0; axis < AXES; axis++) {
    a_delta += square(a_unit[axis] * JUNCTION_DEVIATION);
    b_delta += square(b_unit[axis] * JUNCTION_DEVIATION);
  }

  if (!a_delta || !b_delta) return 0; // One or both unit vectors are null

  float delta = (sqrt(a_delta) + sqrt(b_delta)) / 2;
  float sintheta_over2 = sqrt((1 - costheta) / 2);
  float radius = delta * sintheta_over2 / (1 - sintheta_over2);
  float velocity = sqrt(radius * JUNCTION_ACCELERATION);

  return velocity;
}


/* Find the axis for which the jerk cannot be exceeded - the 'jerk_axis'.
 * This is the axis for which the time to decelerate from the target velocity
 * to zero would be the longest.
 *
 * We can determine the "longest" deceleration in terms of time or distance.
 *
 * The math for time-to-decelerate T from speed S to speed E with constant
 * jerk J is:
 *
 *   T = 2 * sqrt((S - E) / J)
 *
 * Since E is always zero in this calculation, we can simplify:
 *
 *   T = 2 * sqrt(S / J)
 *
 * The math for distance-to-decelerate l from speed S to speed E with
 * constant jerk J is:
 *
 *   l = (S + E) * sqrt((S - E) / J)
 *
 * Since E is always zero in this calculation, we can simplify:
 *
 *   l = S * sqrt(S / J)
 *
 * The final value we want to know is which one is *longest*, compared to the
 * others, so we don't need the actual value.  This means that the scale of
 * the value doesn't matter, so for T we can remove the "2 *" and for L we can
 * remove the "S *".  This leaves both to be simply "sqrt(S / J)".  Since we
 * don't need the scale, it doesn't matter what speed we're coming from, so S
 * can be replaced with 1.
 *
 * However, we *do* need to compensate for the fact that each axis contributes
 * differently to the move, so we will scale each contribution C[n] by the
 * proportion of the axis movement length D[n].  Using that, we construct the
 * following, with these definitions:
 *
 *   J[n] = max_jerk for axis n
 *   D[n] = distance traveled for this move for axis n
 *   C[n] = "axis contribution" of axis n
 *
 * For each axis n:
 *
 *   C[n] = sqrt(1 / J[n]) * D[n]
 *
 * Keeping in mind that we only need a rank compared to the other axes, we can
 * further optimize the calculations:
 *
 * Square the expression to remove the square root:
 *
 *   C[n]^2 = 1 / J[n] * D[n]^2
 *
 * We don't care that C is squared, so we'll use it that way.  Also note that
 * we already have 1 / J[n] calculated for each axis.
 */
int mp_find_jerk_axis(const float axis_square[]) {
  float C;
  float maxC = 0;
  int jerk_axis = 0;

  for (int axis = 0; axis < AXES; axis++)
    if (axis_square[axis]) { // Do not use fp_ZERO here
      // Squaring axis_length ensures it's positive
      C = axis_square[axis] * axis_get_recip_jerk(axis);

      if (maxC < C) {
        maxC = C;
        jerk_axis = axis;
      }
    }

  return jerk_axis;
}


/// Determine jerk value to use for the block.
static float _calc_jerk(const float axis_square[], const float unit[]) {
  int axis = mp_find_jerk_axis(axis_square);

  // Finally, the selected jerk term needs to be scaled by the
  // reciprocal of the absolute value of the axis's unit
  // vector term.  This way when the move is finally decomposed into
  // its constituent axes for execution the jerk for that axis will be
  // at it's maximum value.
  return axis_get_jerk_max(axis) * JERK_MULTIPLIER / fabs(unit[axis]);
}


/// Compute cached jerk terms used by planning
static void _calc_and_cache_jerk_values(mp_buffer_t *bf) {
  static float jerk = 0;
  static float cbrt_jerk = 0;

  if (JERK_MATCH_PRECISION < fabs(bf->jerk - jerk)) { // Tolerance comparison
    jerk = bf->jerk;
    cbrt_jerk = cbrt(bf->jerk);
  }

  bf->cbrt_jerk = cbrt_jerk;
}


static void _calc_max_velocities(mp_buffer_t *bf, float move_time,
                                 bool exact_stop) {
  ASSERT(0 < move_time && isfinite(move_time));

  float junction_velocity =
    _get_junction_vmax(mp_buffer_prev(bf)->unit, bf->unit);

  bf->cruise_vmax = bf->length / move_time; // target velocity requested
  bf->entry_vmax = min(bf->cruise_vmax, junction_velocity);
  bf->delta_vmax = mp_get_target_velocity(0, bf->length, bf);
  bf->exit_vmax = min(bf->cruise_vmax, (bf->entry_vmax + bf->delta_vmax));
  bf->braking_velocity = bf->delta_vmax;

  // Zero out exact stop cases
  if (exact_stop) bf->entry_vmax = bf->exit_vmax = 0;
  else bf->replannable = true;
}


/* Compute optimal and minimum move times
 *
 * "Minimum time" is the fastest the move can be performed given the velocity
 * constraints on each participating axis - regardless of the feed rate
 * requested. The minimum time is the time limited by the rate-limiting
 * axis. The minimum time is needed to compute the optimal time and is recorded
 * for possible feed override computation.
 *
 * "Optimal time" is either the time resulting from the requested feed rate or
 * the minimum time if the requested feed rate is not achievable. Optimal times
 * for rapids are always the minimum time.
 *
 * The following times are compared and the longest is returned:
 *   - G93 inverse time (if G93 is active)
 *   - time for coordinated move at requested feed rate
 *   - time that the slowest axis would require for the move
 *
 * NIST RS274NGC_v3 Guidance
 *
 * The following is verbatim text from NIST RS274NGC_v3. As I interpret A for
 * moves that combine both linear and rotational movement, the feed rate should
 * apply to the XYZ movement, with the rotational axis (or axes) timed to start
 * and end at the same time the linear move is performed. It is possible under
 * this case for the rotational move to rate-limit the linear move.
 *
 *  2.1.2.5 Feed Rate
 *
 * The rate at which the controlled point or the axes move is nominally a steady
 * rate which may be set by the user. In the Interpreter, the interpretation of
 * the feed rate is as follows unless inverse time feed rate mode is being used
 * in the RS274/NGC view (see Section 3.5.19). The machining functions view of
 * feed rate, as described in Section 4.3.5.1, has conditions under which the
 * set feed rate is applied differently, but none of these is used in the
 * Interpreter.
 *
 * A.  For motion involving one or more of the X, Y, and Z axes (with or without
 *     simultaneous rotational axis motion), the feed rate means length units
 *     per minute along the programmed XYZ path, as if the rotational axes were
 *     not moving.
 *
 * B.  For motion of one rotational axis with X, Y, and Z axes not moving, the
 *     feed rate means degrees per minute rotation of the rotational axis.
 *
 * C.  For motion of two or three rotational axes with X, Y, and Z axes not
 *     moving, the rate is applied as follows. Let dA, dB, and dC be the angles
 *     in degrees through which the A, B, and C axes, respectively, must move.
 *     Let D = sqrt(dA^2 + dB^2 + dC^2). Conceptually, D is a measure of total
 *     angular motion, using the usual Euclidean metric. Let T be the amount of
 *     time required to move through D degrees at the current feed rate in
 *     degrees per minute. The rotational axes should be moved in coordinated
 *     linear motion so that the elapsed time from the start to the end of the
 *     motion is T plus any time required for acceleration or deceleration.
 */
static float _calc_move_time(const float axis_length[],
                             const float axis_square[], bool rapid,
                             bool inverse_time, float feed_rate,
                             float feed_override) {
  ASSERT(0 < feed_override);
  float max_time = 0;

  // Compute times for feed motion
  if (!rapid) {
    if (inverse_time) max_time = feed_rate;
    else {
      // Compute length of linear move in millimeters.  Feed rate in mm/min.
      max_time = sqrt(axis_square[AXIS_X] + axis_square[AXIS_Y] +
                      axis_square[AXIS_Z]) / feed_rate;

      // If no linear axes, compute length of multi-axis rotary move in degrees.
      // Feed rate is provided as degrees/min
      if (fp_ZERO(max_time))
        max_time = sqrt(axis_square[AXIS_A] + axis_square[AXIS_B] +
                        axis_square[AXIS_C]) / feed_rate;
    }
  }

  // Apply feed override
  max_time /= feed_override;

  // Compute time required for rate-limiting axis
  for (int axis = 0; axis < AXES; axis++) {
    float time = fabs(axis_length[axis]) / axis_get_velocity_max(axis);
    if (max_time < time) max_time = time;
  }

  return max_time < MIN_SEGMENT_TIME ? MIN_SEGMENT_TIME : max_time;
}


/*** Plan line acceleration / deceleration
 *
 * This function uses constant jerk motion equations to plan acceleration and
 * deceleration.  Jerk is the rate of change of acceleration; it's the 1st
 * derivative of acceleration, and the 3rd derivative of position.  Jerk is a
 * measure of impact to the machine.  Controlling jerk smooths transitions
 * between moves and allows for faster feeds while controlling machine
 * oscillations and other undesirable side-effects.
 *
 * Notes:
 * [1] All math is done in absolute coordinates using single precision floats.
 *
 * [2] Returning a status that is not STAT_OK means the endpoint is NOT
 * advanced.  So lines that are too short to move will accumulate and get
 * executed once the accumulated error exceeds the minimums.
 *
 * @param reed_rate is in minutes when @param inverse_time is true.
 * See mach_set_feed_rate()
 */
stat_t mp_aline(const float target[], bool rapid, bool inverse_time,
                bool exact_stop, float feed_rate, float feed_override,
                int32_t line) {
  DEBUG_CALL("(%f, %f, %f, %f), %s, %s, %s, %f, %f, %d", target[0], target[1],
             target[2], target[3], rapid ? "true" : "false",
             inverse_time ? "true" : "false", exact_stop ? "true" : "false",
             feed_rate, feed_override, line);

  // Compute axis and move lengths
  float axis_length[AXES];
  float axis_square[AXES];
  float length_square = 0;

  for (int axis = 0; axis < AXES; axis++) {
    axis_length[axis] = target[axis] - mp_get_axis_position(axis);
    axis_square[axis] = square(axis_length[axis]);
    length_square += axis_square[axis];
  }

  float length = sqrt(length_square);
  if (fp_ZERO(length)) return STAT_OK;

  // Get a buffer.  Note, new buffers are initialized to zero.
  mp_buffer_t *bf = mp_queue_get_tail(); // current move pointer

  // Set buffer values
  bf->length = length;
  copy_vector(bf->target, target);

  // Compute unit vector
  for (int axis = 0; axis < AXES; axis++)
    bf->unit[axis] = axis_length[axis] / length;

  // Compute and cache jerk values
  bf->jerk = _calc_jerk(axis_square, bf->unit);
  _calc_and_cache_jerk_values(bf);

  // Compute move time and velocities
  float time = _calc_move_time(axis_length, axis_square, rapid, inverse_time,
                               feed_rate, feed_override);
  _calc_max_velocities(bf, time, exact_stop);

  // Note, the following lines must remain in order.
  bf->line = line;              // Planner needs then when planning steps
  mp_plan(bf);                  // Plan block list
  mp_set_position(target);      // Set planner position before committing buffer
  mp_queue_push(mp_exec_aline, line); // After position update

  return STAT_OK;
}
