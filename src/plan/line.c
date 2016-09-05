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

#include "line.h"

#include "planner.h"
#include "exec.h"
#include "runtime.h"
#include "buffer.h"
#include "machine.h"
#include "stepper.h"
#include "util.h"
#include "report.h"

#include <string.h>
#include <stdbool.h>
#include <math.h>

/// common variables for planning (move master)
typedef struct {
  float position[AXES];             // final move position for planning purposes

  float jerk;                       // jerk values cached from previous block
  float recip_jerk;
  float cbrt_jerk;
} mpMoveMaster_t;


mpMoveMaster_t mm = {{0}}; // context for line planning


/// Set planner position for a single axis
void mp_set_planner_position(int axis, const float position) {
  mm.position[axis] = position;
}


/* Sonny's algorithm - simple
 *
 * Computes the maximum allowable junction speed by finding the
 * velocity that will yield the centripetal acceleration in the
 * corner_acceleration value. The value of delta sets the effective
 * radius of curvature. Here's Sonny's (Sungeun K. Jeon's)
 * explanation of what's going on:
 *
 * "First let's assume that at a junction we only look a centripetal
 * acceleration to simply things. At a junction of two lines, let's
 * place a circle such that both lines are tangent to the circle. The
 * circular segment joining the lines represents the path for
 * constant centripetal acceleration. This creates a deviation from
 * the path (let's call this delta), which is the distance from the
 * junction to the edge of the circular segment. Delta needs to be
 * defined, so let's replace the term max_jerk (see note 1) with
 * max_junction_deviation, or "delta". This indirectly sets the
 * radius of the circle, and hence limits the velocity by the
 * centripetal acceleration. Think of the this as widening the race
 * track. If a race car is driving on a track only as wide as a car,
 * it'll have to slow down a lot to turn corners. If we widen the
 * track a bit, the car can start to use the track to go into the
 * turn. The wider it is, the faster through the corner it can go.
 *
 * (Note 1: "max_jerk" refers to the old grbl/marlin max_jerk"
 * approximation term, not the TinyG jerk terms)
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
 *     cos(theta) = dot(a, b) / (norm(a) * norm(b)).
 *
 * Most of these calculations are already done in the planner.
 * To remove the acos() and sin() computations, use the trig half
 * angle identity:
 *
 *     sin(theta/2) = +/-sqrt((1 - cos(theta)) / 2)
 *
 * For our applications, this should always be positive. Now just
 * plug the equations into the centripetal acceleration equation:
 * v_c = sqrt(a_max*R). You'll see that there are only two sqrt
 * computations and no sine/cosines."
 *
 * How to compute the radius using brute-force trig:
 *
 *    float theta = acos(costheta);
 *    float radius = delta * sin(theta/2) / (1 - sin(theta/2));
 *
 * This version extends Chamnit's algorithm by computing a value for
 * delta that takes the contributions of the individual axes in the
 * move into account. This allows the control radius to vary by
 * axis. This is necessary to support axes that have different
 * dynamics; such as a Z axis that doesn't move as fast as X and Y
 * (such as a screw driven Z axis on machine with a belt driven XY -
 * like a Shapeoko), or rotary axes ABC that have completely
 * different dynamics than their linear counterparts.
 *
 * The function takes the absolute values of the sum of the unit
 * vector components as a measure of contribution to the move, then
 * scales the delta values from the non-zero axes into a composite
 * delta to be used for the move. Shown for an XY vector:
 *
 *     U[i]    Unit sum of i'th axis    fabs(unit_a[i]) + fabs(unit_b[i])
 *     Usum    Length of sums           Ux + Uy
 *     d       Delta of sums            (Dx * Ux + DY * UY) / Usum
 */
static float _get_junction_vmax(const float a_unit[], const float b_unit[]) {
  float costheta = 0;
  for (int axis = 0; axis < AXES; axis++)
    costheta -= a_unit[axis] * b_unit[axis];

  if (costheta < -0.99) return 10000000;         // straight line cases
  if (costheta > 0.99)  return 0;                // reversal cases

  // Fuse the junction deviations into a vector sum
  float a_delta = 0;
  float b_delta = 0;

  for (int axis = 0; axis < AXES; axis++) {
    a_delta += square(a_unit[axis] * mach.a[axis].junction_dev);
    b_delta += square(b_unit[axis] * mach.a[axis].junction_dev);
  }

  float delta = (sqrt(a_delta) + sqrt(b_delta)) / 2;
  float sintheta_over2 = sqrt((1 - costheta) / 2);
  float radius = delta * sintheta_over2 / (1 - sintheta_over2);
  float velocity = sqrt(radius * JUNCTION_ACCELERATION);

  return velocity;
}


/*** Plan a line with acceleration / deceleration
 *
 * This function uses constant jerk motion equations to plan
 * acceleration and deceleration. Jerk is the rate of change of
 * acceleration; it's the 1st derivative of acceleration, and the
 * 3rd derivative of position. Jerk is a measure of impact to the
 * machine.  Controlling jerk smooths transitions between moves and
 * allows for faster feeds while controlling machine oscillations
 * and other undesirable side-effects.
 *
 * Note All math is done in absolute coordinates using single
 * precision floating point (float).
 *
 * Note: Returning a status that is not STAT_OK means the endpoint
 * is NOT advanced. So lines that are too short to move will
 * accumulate and get executed once the accumulated error exceeds
 * the minimums.
 */
stat_t mp_aline(MoveState_t *ms) {
  // Compute some reusable terms
  float axis_length[AXES];
  float axis_square[AXES];
  float length_square = 0;

  for (int axis = 0; axis < AXES; axis++) {
    axis_length[axis] = ms->target[axis] - mm.position[axis];
    axis_square[axis] = square(axis_length[axis]);
    length_square += axis_square[axis];
  }

  float length = sqrt(length_square);
  if (fp_ZERO(length)) {
    report_request();
    return STAT_OK;
  }

  // If mach_calc_move_time() says the move will take less than the
  // minimum move time get a more accurate time estimate based on
  // starting velocity and acceleration.  The time of the move is
  // determined by its initial velocity (Vi) and how much acceleration
  // will occur. For this we need to look at the exit velocity of
  // the previous block. There are 3 possible cases:
  //
  //    (1) There is no previous block.
  //        Vi = 0
  //
  //    (2) Previous block is optimally planned.
  //        Vi = previous block's exit_velocity
  //
  //    (3) Previous block is not optimally planned.
  //        Vi <= previous block's entry_velocity + delta_velocity

  // Set move time in state
  mach_calc_move_time(axis_length, axis_square);

  if (ms->move_time < MIN_BLOCK_TIME) {
    // Max velocity change for this move
    float delta_velocity = pow(length, 0.66666666) * mm.cbrt_jerk;
    float entry_velocity = 0; // pre-set as if no previous block

    mpBuf_t *bf = mp_get_run_buffer();
    if (bf) {
      if (bf->replannable) // not optimally planned
        entry_velocity = bf->entry_velocity + bf->delta_vmax;
      else entry_velocity = bf->exit_velocity; // optimally planned
    }

    // Compute execution time for this move
    float move_time = 2 * length / (2 * entry_velocity + delta_velocity);
    if (move_time < MIN_BLOCK_TIME) return STAT_MINIMUM_TIME_MOVE;
  }

  // Get a *cleared* buffer and setup move variables
  // Note, mp_free_run_buffer() initializes all buffer variables to zero
  mpBuf_t *bf = mp_get_write_buffer(); // current move pointer
  if (!bf) return CM_ALARM(STAT_BUFFER_FULL_FATAL); // never fails

  // Register callback to exec function
  bf->bf_func = mp_exec_aline;
  bf->length = length;

  // Copy model state into planner buffer
  memcpy(&bf->ms, ms, sizeof(MoveState_t));

  // Compute the unit vector and find the right jerk to use (combined
  // operations) To determine the jerk value to use for the block we
  // want to find the axis for which the jerk cannot be exceeded - the
  // 'jerk-limit' axis. This is the axis for which the time to
  // decelerate from the target velocity to zero would be the longest.
  //
  // We can determine the "longest" deceleration in terms of time or distance.
  //
  // The math for time-to-decelerate T from speed S to speed E with constant
  // jerk J is:
  //
  //   T = 2 * sqrt((S - E) / J)
  //
  // Since E is always zero in this calculation, we can simplify:
  //
  //   T = 2 * sqrt(S / J)
  //
  // The math for distance-to-decelerate l from speed S to speed E with
  // constant jerk J is:
  //
  //   l = (S + E) * sqrt((S - E) / J)
  //
  // Since E is always zero in this calculation, we can simplify:
  //
  //   l = S * sqrt(S / J)
  //
  // The final value we want to know is which one is *longest*,
  // compared to the others, so we don't need the actual value. This
  // means that the scale of the value doesn't matter, so for T we
  // can remove the "2 *" and For L we can remove the "S*".  This
  // leaves both to be simply "sqrt(S/J)". Since we don't need the
  // scale, it doesn't matter what speed we're coming from, so S can
  // be replaced with 1.
  //
  // However, we *do* need to compensate for the fact that each axis
  // contributes differently to the move, so we will scale each
  // contribution C[n] by the proportion of the axis movement length
  // D[n] to the total length of the move L.  Using that, we
  // construct the following, with these definitions:
  //
  //   J[n] = max_jerk for axis n
  //   D[n] = distance traveled for this move for axis n
  //   L = total length of the move in all axes
  //   C[n] = "axis contribution" of axis n
  //
  // For each axis n:
  //
  //   C[n] = sqrt(1 / J[n]) * (D[n] / L)
  //
  // Keeping in mind that we only need a rank compared to the other
  // axes, we can further optimize the calculations:
  //
  // Square the expression to remove the square root:
  //
  //   C[n]^2 = (1 / J[n]) * (D[n] / L)^2
  //
  // We don't care the C is squared, so we'll use it that way.
  //
  // Re-arranged to optimize divides for pre-calculated values,
  // we create a value M that we compute once:
  //
  //   M = 1 / L^2
  //
  // Then we use it in the calculations for every axis:
  //
  //   C[n] = 1 / J[n] * D[n]^2 * M
  //
  // Also note that we already have 1 / J[n] calculated for each axis,
  // which simplifies it further.
  //
  // Finally, the selected jerk term needs to be scaled by the
  // reciprocal of the absolute value of the jerk-limit axis's unit
  // vector term. This way when the move is finally decomposed into
  // its constituent axes for execution the jerk for that axis will be
  // at it's maximum value.

  float C; // contribution term. C = T * a
  float maxC = 0;
  float recip_L2 = 1 / length_square;
  int jerk_axis = 0;

  for (int axis = 0; axis < AXES; axis++)
    // You cannot use the fp_XXX comparisons here!
    if (fabs(axis_length[axis]) > 0) {
      // compute unit vector term (zeros are already zero)
      bf->unit[axis] = axis_length[axis] / bf->length;
      // squaring axis_length ensures it's positive
      C = axis_square[axis] * recip_L2 * mach.a[axis].recip_jerk;

      if (C > maxC) {
        maxC = C;
        jerk_axis = axis; // also needed for junction vmax calculation
      }
    }

  // set up and pre-compute the jerk terms needed for this round of planning
  bf->jerk = mach.a[jerk_axis].jerk_max * JERK_MULTIPLIER /
    fabs(bf->unit[jerk_axis]); // scale jerk

  // specialized comparison for tolerance of delta
  if (fabs(bf->jerk - mm.jerk) > JERK_MATCH_PRECISION) {
    mm.jerk = bf->jerk; // used before this point next time around
    mm.recip_jerk = 1 / bf->jerk; // compute cached jerk terms used by planning
    mm.cbrt_jerk = cbrt(bf->jerk);
  }

  bf->recip_jerk = mm.recip_jerk;
  bf->cbrt_jerk = mm.cbrt_jerk;

  // exact stop cases already zeroed
  float exact_stop = 0;
  if (mach_get_path_control() != PATH_EXACT_STOP) {
    bf->replannable = true;
    exact_stop = 8675309; // an arbitrarily large floating point number
  }

  // finish up the current block variables
  float junction_velocity = _get_junction_vmax(bf->pv->unit, bf->unit);
  bf->cruise_vmax = bf->length / bf->ms.move_time; // target velocity requested
  bf->entry_vmax = min3(bf->cruise_vmax, junction_velocity, exact_stop);
  bf->delta_vmax = mp_get_target_velocity(0, bf->length, bf);
  bf->exit_vmax =
    min3(bf->cruise_vmax, (bf->entry_vmax + bf->delta_vmax), exact_stop);
  bf->braking_velocity = bf->delta_vmax;

  // NOTE: these next lines must remain in exact order. Position must update
  // before committing the buffer.
  mp_plan_block_list(bf);                   // plan block list
  copy_vector(mm.position, bf->ms.target);  // set the planner position

  // commit current block (must follow the position update)
  mp_commit_write_buffer(ms->line, MOVE_TYPE_ALINE);

  return STAT_OK;
}
