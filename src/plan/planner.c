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

/* Planner Notes
 *
 * The planner works below the machine and above the
 * motor mapping and stepper execution layers. A rudimentary
 * multitasking capability is implemented for long-running commands
 * such as lines, arcs, and dwells.  These functions are coded as
 * non-blocking continuations - which are simple state machines
 * that are re-entered multiple times until a particular operation
 * is complete. These functions have 2 parts - the initial call,
 * which sets up the local context, and callbacks (continuations)
 * that are called from the main loop.
 *
 * One important concept is isolation of the three layers of the
 * data model - the Gcode model (gm), planner model (bf queue & mm),
 * and runtime model (exec.c).
 *
 * The Gcode model is owned by the machine and should only be
 * accessed by mach_xxxx() functions. Data from the Gcode model is
 * transferred to the planner by the mp_xxx() functions called by
 * the machine.
 *
 * The planner should only use data in the planner model. When a
 * move (block) is ready for execution the planner data is
 * transferred to the runtime model, which should also be isolated.
 *
 * Lower-level models should never use data from upper-level models
 * as the data may have changed and lead to unpredictable results.
 */

#include "planner.h"

#include "buffer.h"
#include "machine.h"
#include "stepper.h"
#include "motor.h"
#include "state.h"

#include <string.h>
#include <stdbool.h>
#include <stdio.h>


void planner_init() {mp_init_buffers();}


/*** Flush all moves in the planner
 *
 * Does not affect the move currently running.  Does not affect
 * mm or gm model positions.  This function is designed to be called
 * during a hold to reset the planner.  This function should not usually
 * be directly called.  Call mp_request_flush() instead.
 */
void mp_flush_planner() {mp_init_buffers();}


/* Performs axis mapping & conversion of length units to steps (and deals
 * with inhibited axes)
 *
 * The reason steps are returned as floats (as opposed to, say,
 * uint32_t) is to accommodate fractional steps. stepper.c deals
 * with fractional step values as fixed-point binary in order to get
 * the smoothest possible operation. Steps are passed to the move prep
 * routine as floats and converted to fixed-point binary during queue
 * loading. See stepper.c for details.
 */
void mp_kinematics(const float travel[], float steps[]) {
  // You could insert inverse kinematics transformations here
  // or just use travel directly for Cartesian machines.

  // Map motors to axes and convert length units to steps
  // Most of the conversion math has already been done during config in
  // steps_per_unit() which takes axis travel, step angle and microsteps into
  // account.
  for (int motor = 0; motor < MOTORS; motor++) {
    int axis = motor_get_axis(motor);
    if (mach.a[axis].axis_mode == AXIS_INHIBITED) steps[motor] = 0;
    else steps[motor] = travel[axis] * motor_get_steps_per_unit(motor);
  }
}


// The minimum lengths are dynamic and depend on the velocity.  These
// expressions evaluate to the minimum lengths for the current velocity
// settings. Note: The head and tail lengths are 2 minimum segments, the body
// is 1 min segment.
#define MIN_HEAD_LENGTH \
  (MIN_SEGMENT_TIME_PLUS_MARGIN * (bf->cruise_velocity + bf->entry_velocity))
#define MIN_TAIL_LENGTH \
  (MIN_SEGMENT_TIME_PLUS_MARGIN * (bf->cruise_velocity + bf->exit_velocity))
#define MIN_BODY_LENGTH (MIN_SEGMENT_TIME_PLUS_MARGIN * bf->cruise_velocity)


/*** This rather brute-force and long-ish function sets section lengths
 * and velocities based on the line length and velocities requested.  It
 * modifies the incoming bf buffer and returns accurate head, body and
 * tail lengths, and accurate or reasonably approximate velocities.  We
 * care about accuracy on lengths, less so for velocity, as long as velocity
 * errs on the side of too slow.
 *
 * Note: We need the velocities to be set even for zero-length
 * sections (Note: sections, not moves) so we can compute entry and
 * exits for adjacent sections.
 *
 * Inputs used are:
 *
 *   bf->length            - actual block length, length is never changed
 *   bf->entry_velocity    - requested Ve, entry velocity is never changed
 *   bf->cruise_velocity   - requested Vt, is often changed
 *   bf->exit_velocity     - requested Vx, may change for degenerate cases
 *   bf->cruise_vmax       - used in some comparisons
 *   bf->delta_vmax        - used to degrade velocity of short blocks
 *
 * Variables that may be set/updated are:
 *
 *   bf->entry_velocity    - requested Ve
 *   bf->cruise_velocity   - requested Vt
 *   bf->exit_velocity     - requested Vx
 *   bf->head_length       - bf->length allocated to head
 *   bf->body_length       - bf->length allocated to body
 *   bf->tail_length       - bf->length allocated to tail
 *
 * Note: The following conditions must be met on entry:
 *
 *     bf->length must be non-zero (filter these out upstream)
 *     bf->entry_velocity <= bf->cruise_velocity >= bf->exit_velocity
 *
 * Classes of moves:
 *
 *   Requested-Fit - The move has sufficient length to achieve the
 *     target velocity (cruise velocity).  I.e it will accommodate
 *     the acceleration / deceleration profile in the given length.
 *
 *   Rate-Limited-Fit - The move does not have sufficient length to
 *     achieve target velocity.  In this case the cruise velocity
 *     will be set lower than the requested velocity (incoming
 *     bf->cruise_velocity).  The entry and exit velocities are
 *     satisfied.
 *
 *   Degraded-Fit - The move does not have sufficient length to
 *     transition from the entry velocity to the exit velocity in
 *     the available length. These velocities are not negotiable,
 *     so a degraded solution is found.
 *
 *     In worst cases, the move cannot be executed as the required
 *     execution time is less than the minimum segment time.  The
 *     first degradation is to reduce the move to a body-only
 *     segment with an average velocity.  If that still doesn't fit
 *     then the move velocity is reduced so it fits into a minimum
 *     segment.  This will reduce the velocities in that region of
 *     the planner buffer as the moves are replanned to that
 *     worst-case move.
 *
 * Various cases handled (H=head, B=body, T=tail)
 *
 *   Requested-Fit cases:
 *
 *       HBT  Ve<Vt>Vx    sufficient length exists for all parts (corner
 *                        case: HBT')
 *       HB   Ve<Vt=Vx    head accelerates to cruise - exits at full speed
 *                        (corner case: H')
 *       BT   Ve=Vt>Vx    enter at full speed & decelerate (corner case: T')
 *       HT   Ve & Vx     perfect fit HT (very rare). May be symmetric or
 *                        asymmetric
 *       H    Ve<Vx       perfect fit H (common, results from planning)
 *       T    Ve>Vx       perfect fit T (common, results from planning)
 *       B    Ve=Vt=Vx    Velocities are close to each other and within
 *                        matching tolerance
 *
 *   Rate-Limited cases - Ve and Vx can be satisfied but Vt cannot:
 *
 *       HT    (Ve=Vx)<Vt    symmetric case. Split the length and compute Vt.
 *       HT'   (Ve!=Vx)<Vt   asymmetric case. Find H and T by successive
 *                           approximation.
 *       HBT'  body length < min body length - treated as an HT case
 *       H'    body length < min body length - subsume body into head length
 *       T'    body length < min body length - subsume body into tail length
 *
 *   Degraded fit cases - line is too short to satisfy both Ve and Vx:
 *
 *       H"    Ve<Vx        Ve is degraded (velocity step). Vx is met
 *       T"    Ve>Vx        Ve is degraded (velocity step). Vx is met
 *       B"    <short>      line is very short but drawable; is treated as a
 *                          body only
 *       F     <too short>  force fit: This block is slowed down until it can
 *                          be executed
 *
 * Note: The order of the cases/tests in the code is important.  Start with
 * the shortest cases first and work up. Not only does this simplify the order
 * of the tests, but it reduces execution time when you need it most - when
 * tons of pathologically short Gcode blocks are being thrown at you.
 */
void mp_calculate_trapezoid(mp_buffer_t *bf) {
  // RULE #1 of mp_calculate_trapezoid(): Don't change bf->length
  //
  // F case: Block is too short - run time < minimum segment time
  // Force block into a single segment body with limited velocities
  // Accept the entry velocity, limit the cruise, and go for the best exit
  // velocity you can get given the delta_vmax (maximum velocity slew)
  // supportable.

  float naive_move_time =
    2 * bf->length / (bf->entry_velocity + bf->exit_velocity); // average

  if (naive_move_time < MIN_SEGMENT_TIME_PLUS_MARGIN) {
    bf->cruise_velocity = bf->length / MIN_SEGMENT_TIME_PLUS_MARGIN;
    bf->exit_velocity = max(0.0, min(bf->cruise_velocity,
                                     (bf->entry_velocity - bf->delta_vmax)));
    bf->body_length = bf->length;
    bf->head_length = 0;
    bf->tail_length = 0;

    // We are violating the jerk value but since it's a single segment move we
    // don't use it.
    return;
  }

  // B" case: Block is short, but fits into a single body segment
  if (naive_move_time <= NOM_SEGMENT_TIME) {
    bf->entry_velocity = bf->pv->exit_velocity;

    if (fp_NOT_ZERO(bf->entry_velocity)) {
      bf->cruise_velocity = bf->entry_velocity;
      bf->exit_velocity = bf->entry_velocity;

    } else {
      bf->cruise_velocity = bf->delta_vmax / 2;
      bf->exit_velocity = bf->delta_vmax;
    }

    bf->body_length = bf->length;
    bf->head_length = 0;
    bf->tail_length = 0;

    // We are violating the jerk value but since it's a single segment move we
    // don't use it.
    return;
  }

  // B case:  Velocities all match (or close enough)
  // This occurs frequently in normal gcode files with lots of short lines.
  // This case is not really necessary, but saves lots of processing time.
  if (((bf->cruise_velocity - bf->entry_velocity) <
       TRAPEZOID_VELOCITY_TOLERANCE) &&
      ((bf->cruise_velocity - bf->exit_velocity) <
       TRAPEZOID_VELOCITY_TOLERANCE)) {
    bf->body_length = bf->length;
    bf->head_length = 0;
    bf->tail_length = 0;

    return;
  }

  // Head-only and tail-only short-line cases
  //   H" and T" degraded-fit cases
  //   H' and T' requested-fit cases where the body residual is less than
  //   MIN_BODY_LENGTH
  bf->body_length = 0;
  float minimum_length =
    mp_get_target_length(bf->entry_velocity, bf->exit_velocity, bf);

  // head-only & tail-only cases
  if (bf->length <= (minimum_length + MIN_BODY_LENGTH)) {
    // tail-only cases (short decelerations)
    if (bf->entry_velocity > bf->exit_velocity) {
      // T" (degraded case)
      if (bf->length < minimum_length)
        bf->entry_velocity =
          mp_get_target_velocity(bf->exit_velocity, bf->length, bf);

      bf->cruise_velocity = bf->entry_velocity;
      bf->tail_length = bf->length;
      bf->head_length = 0;

      return;
    }

    // head-only cases (short accelerations)
    if (bf->entry_velocity < bf->exit_velocity) {
      // H" (degraded case)
      if (bf->length < minimum_length)
        bf->exit_velocity =
          mp_get_target_velocity(bf->entry_velocity, bf->length, bf);

      bf->cruise_velocity = bf->exit_velocity;
      bf->head_length = bf->length;
      bf->tail_length = 0;

      return;
    }
  }

  // Set head and tail lengths for evaluating the next cases
  bf->head_length =
    mp_get_target_length(bf->entry_velocity, bf->cruise_velocity, bf);
  bf->tail_length =
    mp_get_target_length(bf->exit_velocity, bf->cruise_velocity, bf);
  if (bf->head_length < MIN_HEAD_LENGTH) { bf->head_length = 0;}
  if (bf->tail_length < MIN_TAIL_LENGTH) { bf->tail_length = 0;}

  // Rate-limited HT and HT' cases
  if (bf->length < (bf->head_length + bf->tail_length)) { // it's rate limited

    // Symmetric rate-limited case (HT)
    if (fabs(bf->entry_velocity - bf->exit_velocity) <
        TRAPEZOID_VELOCITY_TOLERANCE) {
      bf->head_length = bf->length/2;
      bf->tail_length = bf->head_length;
      bf->cruise_velocity =
        min(bf->cruise_vmax,
            mp_get_target_velocity(bf->entry_velocity, bf->head_length, bf));

      if (bf->head_length < MIN_HEAD_LENGTH) {
        // Convert this to a body-only move
        bf->body_length = bf->length;
        bf->head_length = 0;
        bf->tail_length = 0;

        // Average the entry speed and computed best cruise-speed
        bf->cruise_velocity = (bf->entry_velocity + bf->cruise_velocity)/2;
        bf->entry_velocity = bf->cruise_velocity;
        bf->exit_velocity = bf->cruise_velocity;
      }

      return;
    }

    // Asymmetric HT' rate-limited case. This is relatively expensive but it's
    // not called very often
    float computed_velocity = bf->cruise_vmax;
    do {
      // initialize from previous iteration
      bf->cruise_velocity = computed_velocity;
      bf->head_length =
        mp_get_target_length(bf->entry_velocity, bf->cruise_velocity, bf);
      bf->tail_length =
        mp_get_target_length(bf->exit_velocity, bf->cruise_velocity, bf);

      if (bf->head_length > bf->tail_length) {
        bf->head_length =
          (bf->head_length / (bf->head_length + bf->tail_length)) * bf->length;
        computed_velocity =
          mp_get_target_velocity(bf->entry_velocity, bf->head_length, bf);

      } else {
        bf->tail_length =
          (bf->tail_length / (bf->head_length + bf->tail_length)) * bf->length;
        computed_velocity =
          mp_get_target_velocity(bf->exit_velocity, bf->tail_length, bf);
      }

    } while ((fabs(bf->cruise_velocity - computed_velocity) /
              computed_velocity) > TRAPEZOID_ITERATION_ERROR_PERCENT);

    // set velocity and clean up any parts that are too short
    bf->cruise_velocity = computed_velocity;
    bf->head_length =
      mp_get_target_length(bf->entry_velocity, bf->cruise_velocity, bf);
    bf->tail_length = bf->length - bf->head_length;

    if (bf->head_length < MIN_HEAD_LENGTH) {
      // adjust the move to be all tail...
      bf->tail_length = bf->length;
      bf->head_length = 0;
    }

    if (bf->tail_length < MIN_TAIL_LENGTH) {
      bf->head_length = bf->length;            //...or all head
      bf->tail_length = 0;
    }

    return;
  }

  // Requested-fit cases: remaining of: HBT, HB, BT, BT, H, T, B, cases
  bf->body_length = bf->length - bf->head_length - bf->tail_length;

  // If a non-zero body is < minimum length distribute it to the head and/or
  // tail. This will generate small (acceptable) velocity errors in runtime
  // execution but preserve correct distance, which is more important.
  if ((bf->body_length < MIN_BODY_LENGTH) && (fp_NOT_ZERO(bf->body_length))) {
    if (fp_NOT_ZERO(bf->head_length)) {
      if (fp_NOT_ZERO(bf->tail_length)) {            // HBT reduces to HT
        bf->head_length += bf->body_length / 2;
        bf->tail_length += bf->body_length / 2;

      } else bf->head_length += bf->body_length;     // HB reduces to H
    } else bf->tail_length += bf->body_length;       // BT reduces to T

    bf->body_length = 0;

    // If the body is a standalone make the cruise velocity match the entry
    // velocity. This removes a potential velocity discontinuity at the expense
    // of top speed
  } else if ((fp_ZERO(bf->head_length)) && (fp_ZERO(bf->tail_length)))
    bf->cruise_velocity = bf->entry_velocity;
}


/*** Plans the entire block list
 *
 * The block list is the circular buffer of planner buffers
 * (bf's). The block currently being planned is the "bf" block.  The
 * "first block" is the next block to execute; queued immediately
 * behind the currently executing block, aka the "running" block.
 * In some cases, there is no first block because the list is empty
 * or there is only one block and it is already running.
 *
 * If blocks following the first block are already optimally
 * planned (non replannable) the first block that is not optimally
 * planned becomes the effective first block.
 *
 * mp_plan_block_list() plans all blocks between and including the
 * (effective) first block and the bf.  It sets entry, exit and
 * cruise v's from vmax's then calls trapezoid generation.
 *
 * Variables that must be provided in the mp_buffer_ts that will be processed:
 *
 *   bf (function arg)     - end of block list (last block in time)
 *   bf->replannable       - start of block list set by last FALSE value
 *                           [Note 1]
 *   bf->move_type         - typically MOVE_TYPE_ALINE. Other move_types should
 *                           be set to length=0, entry_vmax=0 and exit_vmax=0
 *                           and are treated as a momentary stop (plan to zero
 *                           and from zero).
 *   bf->length            - provides block length
 *   bf->entry_vmax        - used during forward planning to set entry velocity
 *   bf->cruise_vmax       - used during forward planning to set cruise velocity
 *   bf->exit_vmax         - used during forward planning to set exit velocity
 *   bf->delta_vmax        - used during forward planning to set exit velocity
 *   bf->recip_jerk        - used during trapezoid generation
 *   bf->cbrt_jerk         - used during trapezoid generation
 *
 * Variables that will be set during processing:
 *
 *   bf->replannable       - set if the block becomes optimally planned
 *   bf->braking_velocity  - set during backward planning
 *   bf->entry_velocity    - set during forward planning
 *   bf->cruise_velocity   - set during forward planning
 *   bf->exit_velocity     - set during forward planning
 *   bf->head_length       - set during trapezoid generation
 *   bf->body_length       - set during trapezoid generation
 *   bf->tail_length       - set during trapezoid generation
 *
 * Variables that are ignored but here's what you would expect them to be:
 *
 *   bf->run_state         - NEW for all blocks but the earliest
 *   bf->ms.target[]       - block target position
 *   bf->unit[]            - block unit vector
 *   bf->time              - gets set later
 *   bf->jerk              - source of the other jerk variables.
 *
 * Notes:
 *
 * [1] Whether or not a block is planned is controlled by the
 *     bf->replannable setting (set TRUE if it should be). Replan flags
 *     are checked during the backwards pass and prune the replan list
 *     to include only the the latest blocks that require planning
 *
 *     In normal operation the first block (currently running
 *     block) is not replanned, but may be for feedholds and feed
 *     overrides.  In these cases the prep routines modify the
 *     contents of the (ex) buffer and re-shuffle the block list,
 *     re-enlisting the current bf buffer with new parameters.
 *     These routines also set all blocks in the list to be
 *     replannable so the list can be recomputed regardless of
 *     exact stops and previous replanning optimizations.
 */
void mp_plan_block_list(mp_buffer_t *bf) {
  mp_buffer_t *bp = bf;

  // Backward planning pass.  Find first block and update braking velocities.
  // By the end bp points to the buffer before the first block.
  while ((bp = mp_get_prev_buffer(bp)) != bf) {
    if (!bp->replannable) break;
    bp->braking_velocity =
      min(bp->nx->entry_vmax, bp->nx->braking_velocity) + bp->delta_vmax;
  }

  // Forward planning pass.  Recompute trapezoids from the first block to bf.
  while ((bp = mp_get_next_buffer(bp)) != bf) {
    if (bp->pv == bf) bp->entry_velocity = bp->entry_vmax; // first block
    else bp->entry_velocity = bp->pv->exit_velocity; // other blocks

    bp->cruise_velocity = bp->cruise_vmax;
    bp->exit_velocity = min4(bp->exit_vmax, bp->nx->entry_vmax,
                             bp->nx->braking_velocity,
                             bp->entry_velocity + bp->delta_vmax);

    mp_calculate_trapezoid(bp);

    // Test for optimally planned trapezoids by checking exit conditions
    if  ((fp_EQ(bp->exit_velocity, bp->exit_vmax) ||
          fp_EQ(bp->exit_velocity, bp->nx->entry_vmax)) ||
         (!bp->pv->replannable &&
          fp_EQ(bp->exit_velocity, (bp->entry_velocity + bp->delta_vmax))))
      bp->replannable = false;
  }

  // Finish last block
  bp->entry_velocity = bp->pv->exit_velocity;
  bp->cruise_velocity = bp->cruise_vmax;
  bp->exit_velocity = 0;

  mp_calculate_trapezoid(bp);
}


void mp_replan_blocks() {
  mp_buffer_t *bf = mp_get_run_buffer();
  if (!bf) return;

  mp_buffer_t *bp = bf;

  // Mark all blocks replanable
  while (true) {
    bp->replannable = true;
    if (bp->nx->run_state == MOVE_OFF || bp->nx == bf) break;
    bp = mp_get_next_buffer(bp);
  }

  // Plan blocks
  mp_plan_block_list(bp);
}


/*** Derive accel/decel length from delta V and jerk
 *
 * A convenient function for determining the optimal_length (L) of a line
 * given the initial velocity (Vi), final velocity (Vf) and maximum jerk (Jm).
 *
 * Definitions:
 *
 *   Jm = the given maximum jerk
 *   T  = time of the entire move
 *   Vi = initial velocity
 *   Vf = final velocity
 *   T  = time
 *   As = accel at inflection point between convex & concave portions of S-curve
 *   Ar = ramp acceleration
 *
 * Formulas:
 *
 *   T  = 2 * sqrt((Vt - Vi) / Jm)
 *   As = (Jm * T) / 2
 *   Ar = As / 2 = (Jm * T) / 4
 *
 * Then the length (distance) equation is:
 *
 *    a)  L = (Vf - Vi) * T - (Ar * T^2) / 2
 *
 * Substituting for Ar and T:
 *
 *    b)  L = (Vf - Vi) * 2 * sqrt((Vf - Vi) / Jm) -
 *            (2 * sqrt((Vf - Vi) / Jm) * (Vf - Vi)) / 2
 *
 * Reducing b).  See Wolfram Alpha:
 *
 *    c)  L = (Vf - Vi)^(3/2) / sqrt(Jm)
 *
 * Assuming Vf >= Vi [Note 2]:
 *
 *    d)  L = (Vf - Vi) * sqrt((Vf - Vi) / Jm)
 *
 * Notes:
 *
 * [1] Assuming Vi, Vf and L are positive or zero.
 * [2] Cannot assume Vf >= Vi due to rounding errors and use of
 *     PLANNER_VELOCITY_TOLERANCE necessitating the introduction of fabs()
 */
float mp_get_target_length(float Vi, float Vf, const mp_buffer_t *bf) {
  return fabs(Vi - Vf) * sqrt(fabs(Vi - Vf) * bf->recip_jerk);
}


#define GET_VELOCITY_ITERATIONS 0 // must be zero or more

/*** Derive velocity achievable from delta V and length
 *
 * A convenient function for determining Vf target velocity for a given
 * initial velocity (Vi), length (L), and maximum jerk (Jm).  Equation e) is
 * b) solved for Vf.  Equation f) is c) solved for Vf.  Use f) (obviously)
 *
 *   e)  Vf = (sqrt(L) * (L / sqrt(1 / Jm))^(1/6) +
 *            (1 / Jm)^(1/4) * Vi) / (1 / Jm)^(1/4)
 *
 *   f)  Vf = L^(2/3) * Jm^(1/3) + Vi
 *
 * FYI: Here's an expression that returns the jerk for a given deltaV and L:
 *
 *   cube(deltaV / (pow(L, 0.66666666)));
 *
 * We do some Newton-Raphson iterations to narrow it down.
 * We need a formula that includes known variables except the one we want to
 * find, and has a root [Z(x) = 0] at the value (x) we are looking for.
 *
 *   Z(x) = zero at x
 *
 * We calculate the value from the knowns and the estimate (see below) and then
 * subtract the known value to get zero (root) if x is the correct value.
 *
 *   Vi = initial velocity (known)
 *   Vf = estimated final velocity
 *   J  = jerk (known)
 *   L  = length (know)
 *
 * There are (at least) two such functions we can use:
 *
 * L from J, Vi, and Vf:
 *
 *   L = sqrt((Vf - Vi) / J) * (Vi + Vf)
 *
 * Replacing Vf with x, and subtracting the known L we get:
 *
 *   0 = sqrt((x - Vi) / J) * (Vi + x) - L
 *   Z(x) = sqrt((x - Vi) / J) * (Vi + x) - L
 *
 * Or J from L, Vi, and Vf:
 *
 *   J = ((Vf - Vi) * (Vi + Vf)^2) / L^2
 *
 * Replacing Vf with x, and subtracting the known J we get:
 *
 *   0 = ((x - Vi) * (Vi + x)^2) / L^2 - J
 *   Z(x) = ((x - Vi) * (Vi + x)^2) / L^2 - J
 *
 * L doesn't resolve to the value very quickly (its graph is nearly vertical).
 * So, we'll use J, which resolves in < 10 iterations, often in only two or
 * three with a good estimate.
 *
 * In order to do a Newton-Raphson iteration, we need the derivative. Here
 * they are for both the (unused) L and the (used) J formulas above:
 *
 *   J > 0, Vi > 0, Vf > 0
 *   A = sqrt((x - Vi) * J)
 *   B = sqrt((x - Vi) / J)
 *
 *   L'(x) = B + (Vi + x) / (2 * J) + (Vi + x) / (2 * A)
 *
 *   J'(x) = (2 * Vi * x - Vi^2 + 3 * x^2) / L^2
 */
float mp_get_target_velocity(float Vi, float L, const mp_buffer_t *bf) {
  // 0 iterations (a reasonable estimate)
  float x = pow(L, 0.66666666) * bf->cbrt_jerk + Vi; // First estimate

#if (GET_VELOCITY_ITERATIONS > 0)
  const float L2 = L * L;
  const float Vi2 = Vi * Vi;

  for (int i = 0; i < GET_VELOCITY_ITERATIONS; i++)
    // x' = x - Z(x) / J'(x)
    x = x - ((x - Vi) * square(Vi + x) / L2 - bf->jerk) /
      ((2 * Vi * x - Vi2 + 3 * x * x) / L2);
#endif

  return x;
}
