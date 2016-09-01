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

#include "planner.h"
#include "buffer.h"
#include "stepper.h"
#include "motor.h"
#include "util.h"
#include "report.h"
#include "state.h"
#include "config.h"

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>


/*** Segment runner
 *
 * Notes on step error correction:
 *
 * The commanded_steps are the target_steps delayed by one more
 * segment.  This lines them up in time with the encoder readings so a
 * following error can be generated
 *
 * The following_error term is positive if the encoder reading is
 * greater than (ahead of) the commanded steps, and negative (behind)
 * if the encoder reading is less than the commanded steps. The
 * following error is not affected by the direction of movement - it's
 * purely a statement of relative position. Examples:
 *
 *    Encoder Commanded   Following Err
 *     100      90        +10    encoder is 10 steps ahead of commanded steps
 *     -90    -100        +10    encoder is 10 steps ahead of commanded steps
 *      90     100        -10    encoder is 10 steps behind commanded steps
 *    -100     -90        -10    encoder is 10 steps behind commanded steps
 */
static stat_t _exec_aline_segment() {
  float travel_steps[MOTORS];

  // Set target position for the segment
  // If the segment ends on a section waypoint, synchronize to the
  // head, body or tail end.  Otherwise, if not at a section waypoint
  // compute target from segment time and velocity.  Don't do waypoint
  // correction if you are going into a hold.
  if (!--mr.segment_count && !mr.section_new && mp_get_state() == STATE_RUNNING)
    copy_vector(mr.ms.target, mr.waypoint[mr.section]);

  else {
    float segment_length = mr.segment_velocity * mr.segment_time;

    for (int i = 0; i < AXES; i++)
      mr.ms.target[i] = mr.position[i] + mr.unit[i] * segment_length;
  }

  // Convert target position to steps.  Bucket-brigade the old target
  // down the chain before getting the new target from kinematics
  //
  // The direct manipulation of steps to compute travel_steps only
  // works for Cartesian kinematics.  Other kinematics may require
  // transforming travel distance as opposed to simply subtracting
  // steps.
  for (int i = 0; i < MOTORS; i++) {
    // previous segment's position, delayed by 1 segment
    mr.commanded_steps[i] = mr.position_steps[i];
    // previous segment's target becomes position
    mr.position_steps[i] = mr.target_steps[i];
    // current encoder position (aligns to commanded_steps)
    mr.encoder_steps[i] = motor_get_encoder(i);
    mr.following_error[i] = mr.encoder_steps[i] - mr.commanded_steps[i];
  }

  // now determine the target steps
  mp_kinematics(mr.ms.target, mr.target_steps);

  // and compute the distances, in steps, to be traveled
  for (int i = 0; i < MOTORS; i++)
    travel_steps[i] = mr.target_steps[i] - mr.position_steps[i];

  // Call the stepper prep function
  RITORNO(st_prep_line(travel_steps, mr.following_error, mr.segment_time));

  // update position from target
  copy_vector(mr.position, mr.ms.target);

  if (!mr.segment_count) return STAT_OK; // this section has run all segments
  return STAT_EAGAIN; // this section still has more segments to run
}


/*** Forward difference math explained:
 *
 * We are using a quintic (fifth-degree) Bezier polynomial for the
 * velocity curve.  This gives us a "linear pop" velocity curve;
 * with pop being the sixth derivative of position: velocity - 1st,
 * acceleration - 2nd, jerk - 3rd, snap - 4th, crackle - 5th, pop - 6th
 *
 * The Bezier curve takes the form:
 *
 *   V(t) = P_0 * B_0(t) + P_1 * B_1(t) + P_2 * B_2(t) + P_3 * B_3(t) +
 *          P_4 * B_4(t) + P_5 * B_5(t)
 *
 * Where 0 <= t <= 1, and V(t) is the velocity. P_0 through P_5 are
 * the control points, and B_0(t) through B_5(t) are the Bernstein
 * basis as follows:
 *
 *   B_0(t) =   (1 - t)^5        =   -t^5 +  5t^4 - 10t^3 + 10t^2 -  5t + 1
 *   B_1(t) =  5(1 - t)^4 * t    =   5t^5 - 20t^4 + 30t^3 - 20t^2 +  5t
 *   B_2(t) = 10(1 - t)^3 * t^2  = -10t^5 + 30t^4 - 30t^3 + 10t^2
 *   B_3(t) = 10(1 - t)^2 * t^3  =  10t^5 - 20t^4 + 10t^3
 *   B_4(t) =  5(1 - t)   * t^4  =  -5t^5 +  5t^4
 *   B_5(t) =               t^5  =    t^5
 *
 *                                      ^       ^       ^       ^     ^   ^
 *                                      A       B       C       D     E   F
 *
 * We use forward-differencing to calculate each position through the curve.
 * This requires a formula of the form:
 *
 *   V_f(t) = A * t^5 + B * t^4 + C * t^3 + D * t^2 + E * t + F
 *
 * Looking at the above B_0(t) through B_5(t) expanded forms, if we
 * take the coefficients of t^5 through t of the Bezier form of V(t),
 * we can determine that:
 *
 *   A =      -P_0 +  5 * P_1 - 10 * P_2 + 10 * P_3 -  5 * P_4 +  P_5
 *   B =   5 * P_0 - 20 * P_1 + 30 * P_2 - 20 * P_3 +  5 * P_4
 *   C = -10 * P_0 + 30 * P_1 - 30 * P_2 + 10 * P_3
 *   D =  10 * P_0 - 20 * P_1 + 10 * P_2
 *   E = - 5 * P_0 +  5 * P_1
 *   F =       P_0
 *
 * Now, since we will (currently) *always* want the initial
 * acceleration and jerk values to be 0, We set P_i = P_0 = P_1 =
 * P_2 (initial velocity), and P_t = P_3 = P_4 = P_5 (target
 * velocity), which, after simplification, resolves to:
 *
 *   A = - 6 * P_i +  6 * P_t
 *   B =  15 * P_i - 15 * P_t
 *   C = -10 * P_i + 10 * P_t
 *   D = 0
 *   E = 0
 *   F = P_i
 *
 * Given an interval count of I to get from P_i to P_t, we get the
 * parametric "step" size of h = 1/I.  We need to calculate the
 * initial value of forward differences (F_0 - F_5) such that the
 * inital velocity V = P_i, then we iterate over the following I
 * times:
 *
 *   V   += F_5
 *   F_5 += F_4
 *   F_4 += F_3
 *   F_3 += F_2
 *   F_2 += F_1
 *
 * See
 * http://www.drdobbs.com/forward-difference-calculation-of-bezier/184403417
 * for an example of how to calculate F_0 - F_5 for a cubic bezier
 * curve. Since this is a quintic bezier curve, we need to extend
 * the formulas somewhat.  I'll not go into the long-winded
 * step-by-step here, but it gives the resulting formulas:
 *
 *   a = A, b = B, c = C, d = D, e = E, f = F
 *
 *   F_5(t + h) - F_5(t) = (5ah)t^4 + (10ah^2 + 4bh)t^3 +
 *     (10ah^3 + 6bh^2 + 3ch)t^2 + (5ah^4 + 4bh^3 + 3ch^2 + 2dh)t + ah^5 +
 *     bh^4 + ch^3 + dh^2 + eh
 *
 *   a = 5ah
 *   b = 10ah^2 + 4bh
 *   c = 10ah^3 + 6bh^2 + 3ch
 *   d = 5ah^4 + 4bh^3 + 3ch^2 + 2dh
 *
 * After substitution, simplification, and rearranging:
 *
 *   F_4(t + h) - F_4(t) = (20ah^2)t^3 + (60ah^3 + 12bh^2)t^2 +
 *     (70ah^4 + 24bh^3 + 6ch^2)t + 30ah^5 + 14bh^4 + 6ch^3 + 2dh^2
 *
 *   a = 20ah^2
 *   b = 60ah^3 + 12bh^2
 *   c = 70ah^4 + 24bh^3 + 6ch^2
 *
 * After substitution, simplification, and rearranging:
 *
 *   F_3(t + h) - F_3(t) = (60ah^3)t^2 + (180ah^4 + 24bh^3)t + 150ah^5 +
 *     36bh^4 + 6ch^3
 *
 * You get the picture...
 *
 *   F_2(t + h) - F_2(t) = (120ah^4)t + 240ah^5 + 24bh^4
 *   F_1(t + h) - F_1(t) = 120ah^5
 *
 * Normally, we could then assign t = 0, use the A-F values from
 * above, and get out initial F_* values.  However, for the sake of
 * "averaging" the velocity of each segment, we actually want to have
 * the initial V be be at t = h/2 and iterate I-1 times.  So, the
 * resulting F_* values are (steps not shown):
 *
 *   F_5 = 121Ah^5 / 16 + 5Bh^4 + 13Ch^3 / 4 + 2Dh^2 + Eh
 *   F_4 = 165Ah^5 / 2 + 29Bh^4 + 9Ch^3 + 2Dh^2
 *   F_3 = 255Ah^5 + 48Bh^4 + 6Ch^3
 *   F_2 = 300Ah^5 + 24Bh^4
 *   F_1 = 120Ah^5
 *
 * Note that with our current control points, D and E are actually 0.
 */
static float _init_forward_diffs(float Vi, float Vt, float segments) {
  float A =  -6.0 * Vi +  6.0 * Vt;
  float B =  15.0 * Vi - 15.0 * Vt;
  float C = -10.0 * Vi + 10.0 * Vt;
  // D = 0
  // E = 0
  // F = Vi

  float h = 1 / segments;

  float Ah_5 = A * h * h * h * h * h;
  float Bh_4 = B * h * h * h * h;
  float Ch_3 = C * h * h * h;

  mr.forward_diff[4] = 121.0 / 16.0 * Ah_5 + 5.0 * Bh_4 + 13.0 / 4.0 * Ch_3;
  mr.forward_diff[3] = 165.0 / 2.0 * Ah_5 + 29.0 * Bh_4 + 9.0 * Ch_3;
  mr.forward_diff[2] = 255.0 * Ah_5 + 48.0 * Bh_4 + 6.0 * Ch_3;
  mr.forward_diff[1] = 300.0 * Ah_5 + 24.0 * Bh_4;
  mr.forward_diff[0] = 120.0 * Ah_5;

  // Calculate the initial velocity by calculating V(h / 2)
  float half_h = h / 2.0;
  float half_Ch_3 = C * half_h * half_h * half_h;
  float half_Bh_4 = B * half_h * half_h * half_h * half_h;
  float half_Ah_5 = C * half_h * half_h * half_h * half_h * half_h;

  return half_Ah_5 + half_Bh_4 + half_Ch_3 + Vi;
}


/// Common code for head and tail sections
static stat_t _exec_aline_section(float length, float vin, float vout) {
  if (mr.section_new) {
    if (fp_ZERO(length)) return STAT_OK; // end the move

    // len / avg. velocity
    mr.ms.move_time = 2 * length / (vin + vout);
    mr.segments = ceil(uSec(mr.ms.move_time) / NOM_SEGMENT_USEC);
    mr.segment_time = mr.ms.move_time / mr.segments;
    mr.segment_count = (uint32_t)mr.segments;

    if (vin == vout) mr.segment_velocity = vin;
    else mr.segment_velocity = _init_forward_diffs(vin, vout, mr.segments);

    if (mr.segment_time < MIN_SEGMENT_TIME)
      return STAT_MINIMUM_TIME_MOVE; // exit /wo advancing position

    // Do first segment
    if (_exec_aline_segment() == STAT_OK) {
      mr.section_new = false;
      return STAT_OK;
    }

    mr.section_new = false;

  } else { // Do subsequent segments
    if (vin != vout) mr.segment_velocity += mr.forward_diff[4];

    if (_exec_aline_segment() == STAT_OK) return STAT_OK;

    if (vin != vout) {
      mr.forward_diff[4] += mr.forward_diff[3];
      mr.forward_diff[3] += mr.forward_diff[2];
      mr.forward_diff[2] += mr.forward_diff[1];
      mr.forward_diff[1] += mr.forward_diff[0];
    }
  }

  return STAT_EAGAIN;
}


/// Callback for tail section
static stat_t _exec_aline_tail() {
  mr.section = SECTION_TAIL;
  return
    _exec_aline_section(mr.tail_length, mr.cruise_velocity, mr.exit_velocity);
}


/// Callback for body section
static stat_t _exec_aline_body() {
  mr.section = SECTION_BODY;

  stat_t status =
    _exec_aline_section(mr.body_length, mr.cruise_velocity, mr.cruise_velocity);

  if (status == STAT_OK) {
    if (mr.section_new) return _exec_aline_tail();

    mr.section = SECTION_TAIL;
    mr.section_new = true;

    return STAT_EAGAIN;
  }

  return status;
}


/// Callback for head section
static stat_t _exec_aline_head() {
  mr.section = SECTION_HEAD;
  stat_t status =
    _exec_aline_section(mr.head_length, mr.entry_velocity, mr.cruise_velocity);

  if (status == STAT_OK) {
    if (mr.section_new) return _exec_aline_body();

    mr.section = SECTION_BODY;
    mr.section_new = true;

    return STAT_EAGAIN;
  }

  return status;
}


/// Initializes move runtime with a new planner buffer
static stat_t _exec_aline_init(mpBuf_t *bf) {
  // Stop here if holding
  if (mp_get_hold_state() == FEEDHOLD_HOLD) return STAT_NOOP;

  // copy in the gcode model state
  memcpy(&mr.ms, &bf->ms, sizeof(MoveState_t));
  bf->replannable = false;
  report_request(); // Executing line number has changed

  // Remove zero length lines.  Short lines have already been removed.
  if (fp_ZERO(bf->length)) {
    mr.active = false; // reset mr buffer
    bf->nx->replannable = false; // prevent overplanning (Note 2)
    mp_free_run_buffer(); // free buffer

    return STAT_NOOP;
  }

  // Initialize move runtime
  bf->move_state = MOVE_RUN;
  mr.active = true;
  mr.section = SECTION_HEAD;
  mr.section_new = true;

  copy_vector(mr.unit, bf->unit);
  copy_vector(mr.final_target, bf->ms.target);

  mr.head_length = bf->head_length;
  mr.body_length = bf->body_length;
  mr.tail_length = bf->tail_length;
  mr.entry_velocity = bf->entry_velocity;
  mr.cruise_velocity = bf->cruise_velocity;
  mr.exit_velocity = bf->exit_velocity;
  mr.jerk = bf->jerk;

  // Generate waypoints for position correction at section ends
  for (int axis = 0; axis < AXES; axis++) {
    mr.waypoint[SECTION_HEAD][axis] =
      mr.position[axis] + mr.unit[axis] * mr.head_length;

    mr.waypoint[SECTION_BODY][axis] =
      mr.position[axis] + mr.unit[axis] * (mr.head_length + mr.body_length);

    mr.waypoint[SECTION_TAIL][axis] =
      mr.position[axis] + mr.unit[axis] *
      (mr.head_length + mr.body_length + mr.tail_length);
  }

  return STAT_OK;
}


/* Aline execution routines
 *
 * Everything here fires from interrupts and must be interrupt safe
 *
 * Returns:
 *
 *   STAT_OK        move is done
 *   STAT_EAGAIN    move is not finished - has more segments to run
 *   STAT_NOOP      cause no stepper operation - do not load the move
 *   STAT_xxxxx     fatal error.  Ends the move and frees the bf buffer
 *
 * This routine is called from the (LO) interrupt level.  The interrupt
 * sequencing relies on the correct behavior of these routines.
 * Each call to _exec_aline() must execute and prep *one and only one*
 * segment.  If the segment is not the last segment in the bf buffer the
 * _aline() returns STAT_EAGAIN. If it's the last segment it returns
 * STAT_OK. If it encounters a fatal error that would terminate the move it
 * returns a valid error code.
 *
 * Notes:
 *
 * [1] Returning STAT_OK ends the move and frees the bf buffer.
 *     Returning STAT_OK at does NOT advance position meaning
 *     any position error will be compensated by the next move.
 *
 * [2] Solves a potential race condition where the current move ends but
 *     the new move has not started because the previous move is still
 *     being run by the steppers.  Planning can overwrite the new move.
 *
 * Operation:
 *
 * Aline generates jerk-controlled S-curves as per Ed Red's course notes:
 *
 *   http://www.et.byu.edu/~ered/ME537/Notes/Ch5.pdf
 *   http://www.scribd.com/doc/63521608/Ed-Red-Ch5-537-Jerk-Equations
 *
 * A full trapezoid is divided into 5 periods. Periods 1 and 2 are the
 * first and second halves of the acceleration ramp (the concave and convex
 * parts of the S curve in the "head"). Periods 3 and 4 are the first
 * and second parts of the deceleration ramp (the tail). There is also
 * a period for the constant-velocity plateau of the trapezoid (the body).
 * There are many possible degenerate trapezoids where any of the 5 periods
 * may be zero length but note that either none or both of a ramping pair can
 * be zero.
 *
 * The equations that govern the acceleration and deceleration ramps are:
 *
 *   Period 1    V = Vi + Jm * (T^2) / 2
 *   Period 2    V = Vh + As * T - Jm * (T^2) / 2
 *   Period 3    V = Vi - Jm * (T^2) / 2
 *   Period 4    V = Vh + As * T + Jm * (T^2) / 2
 *
 * These routines play some games with the acceleration and move timing
 * to make sure this actually work out.  move_time is the actual time of
 * the move, accel_time is the time value needed to compute the velocity -
 * taking the initial velocity into account (move_time does not need to).
 *
 * State transitions - hierarchical state machine:
 *
 * bf->move_state transitions:
 *
 *  from _NEW to _RUN on first call (sub_state set to _OFF)
 *  from _RUN to _OFF on final call or just remain _OFF
 */
stat_t mp_exec_aline(mpBuf_t *bf) {
  if (bf->move_state == MOVE_OFF) return STAT_NOOP;

  stat_t status = STAT_OK;

  // Start a new move
  if (!mr.active) {
    status = _exec_aline_init(bf);
    if (status != STAT_OK) return status;
  }

  // Main segment dispatch.  From this point on the contents of the bf buffer
  // do not affect execution.
  switch (mr.section) {
  case SECTION_HEAD: status = _exec_aline_head(); break;
  case SECTION_BODY: status = _exec_aline_body(); break;
  case SECTION_TAIL: status = _exec_aline_tail(); break;
  }

  mp_state_hold_callback(status == STAT_OK);

  // There are 3 things that can happen here depending on return conditions:
  //
  //   status        bf->move_state      Description
  //   -----------   --------------      ----------------------------------
  //   STAT_EAGAIN   <don't care>        mr buffer has more segments to run
  //   STAT_OK       MOVE_RUN            mr and bf buffers are done
  //   STAT_OK       MOVE_NEW            mr done; bf must be run again
  //                                     (it's been reused)
  if (status != STAT_EAGAIN) {
    mr.active = false;              // reset mr buffer
    bf->nx->replannable = false;    // prevent overplanning (Note 2)
    // Note, feedhold.c may change bf->move_state to reuse this buffer so it
    // can plan the deceleration.
    if (bf->move_state == MOVE_RUN) mp_free_run_buffer();
  }

  return status;
}


/// Dequeues buffer and executes move callback
stat_t mp_exec_move() {
  if (mp_get_state() == STATE_ESTOPPED) return STAT_MACHINE_ALARMED;

  mpBuf_t *bf = mp_get_run_buffer();
  if (!bf) return STAT_NOOP; // nothing running
  if (!bf->bf_func) return CM_ALARM(STAT_INTERNAL_ERROR);

  return bf->bf_func(bf); // move callback
}
