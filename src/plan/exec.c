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
#include "util.h"
#include "runtime.h"
#include "state.h"
#include "stepper.h"
#include "motor.h"
#include "rtc.h"
#include "usart.h"
#include "config.h"

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>


typedef struct {
  float unit[AXES];         // unit vector for axis scaling & planning
  float final_target[AXES]; // final target, used to correct rounding errors
  float waypoint[3][AXES];  // head/body/tail endpoints for correction

  // copies of bf variables of same name
  float head_length;
  float body_length;
  float tail_length;
  float entry_velocity;
  float cruise_velocity;
  float exit_velocity;

  float segments;           // number of segments in line or arc
  uint32_t segment_count;   // count of running segments
  float segment_velocity;   // computed velocity for aline segment
  float segment_time;       // actual time increment per aline segment
  float forward_diff[5];    // forward difference levels
  bool hold_planned;        // true when a feedhold has been planned
  move_section_t section;   // what section is the move in?
  bool section_new;         // true if it's a new section
} mp_exec_t;


static mp_exec_t ex = {{0}};


static stat_t _exec_aline_segment() {
  float target[AXES];

  // Set target position for the segment.  If the segment ends on a section
  // waypoint, synchronize to the head, body or tail end.  Otherwise, if not at
  // a section waypoint compute target from segment time and velocity.  Don't
  // do waypoint correction if you are going into a hold.
  if (!--ex.segment_count && !ex.section_new && !ex.hold_planned)
    copy_vector(target, ex.waypoint[ex.section]);

  else {
    float segment_length = ex.segment_velocity * ex.segment_time;

    for (int axis = 0; axis < AXES; axis++)
      target[axis] =
        mp_runtime_get_axis_position(axis) + ex.unit[axis] * segment_length;
  }

  mp_runtime_set_velocity(ex.segment_velocity);
  RITORNO(mp_runtime_move_to_target(target, ex.segment_time));

  // Return EAGAIN to continue or OK if this segment is done
  return ex.segment_count ? STAT_EAGAIN : STAT_OK;
}


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
float mp_init_forward_diffs(float fdifs[5], float Vi, float Vt, float s) {
  const float h = 1 / s;
  const float s2 = square(s);
  const float Vdxh5 = (Vt - Vi) * h * h * h * h * h;

  fdifs[4] = (32.5 * s2 -  75.0 * s +   45.375) * Vdxh5;
  fdifs[3] = (90.0 * s2 - 435.0 * s +  495.0  ) * Vdxh5;
  fdifs[2] = (60.0 * s2 - 720.0 * s + 1530.0  ) * Vdxh5;
  fdifs[1] = (          - 360.0 * s + 1800.0  ) * Vdxh5;
  fdifs[0] = (                         720.0  ) * Vdxh5;

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


float mp_next_forward_diff(float fdifs[5]) {
  float delta = fdifs[4];

  fdifs[4] += fdifs[3];
  fdifs[3] += fdifs[2];
  fdifs[2] += fdifs[1];
  fdifs[1] += fdifs[0];

  return delta;
}


/// Common code for head and tail sections
static stat_t _exec_aline_section(float length, float vin, float vout) {
  if (ex.section_new) {
    if (fp_ZERO(length)) return STAT_NOOP; // end the section

    // len / avg. velocity
    float move_time = 2 * length / (vin + vout);
    ex.segments = ceil(move_time * MICROSECONDS_PER_MINUTE / NOM_SEGMENT_USEC);
    ex.segment_time = move_time / ex.segments;
    ex.segment_count = (uint32_t)ex.segments;

    if (vin == vout) ex.segment_velocity = vin;
    else ex.segment_velocity =
           mp_init_forward_diffs(ex.forward_diff, vin, vout, ex.segments);

    if (ex.segment_time < MIN_SEGMENT_TIME)
      return STAT_MINIMUM_TIME_MOVE; // exit /wo advancing position

    // First segment
    if (_exec_aline_segment() == STAT_OK) {
      ex.section_new = false;
      return STAT_OK;
    }

    ex.section_new = false;

  } else {
    if (vin != vout)
      ex.segment_velocity += mp_next_forward_diff(ex.forward_diff);

    // Subsequent segments
    if (_exec_aline_segment() == STAT_OK) return STAT_OK;
  }

  return STAT_EAGAIN;
}


/// Callback for tail section
static stat_t _exec_aline_tail() {
  ex.section = SECTION_TAIL;
  return
    _exec_aline_section(ex.tail_length, ex.cruise_velocity, ex.exit_velocity);
}


/// Callback for body section
static stat_t _exec_aline_body() {
  ex.section = SECTION_BODY;

  stat_t status =
    _exec_aline_section(ex.body_length, ex.cruise_velocity, ex.cruise_velocity);

  switch (status) {
  case STAT_NOOP: return _exec_aline_tail();
  case STAT_OK:
    ex.section = SECTION_TAIL;
    ex.section_new = true;

    return STAT_EAGAIN;

  default: return status;
  }
}


/// Callback for head section
static stat_t _exec_aline_head() {
  ex.section = SECTION_HEAD;
  stat_t status =
    _exec_aline_section(ex.head_length, ex.entry_velocity, ex.cruise_velocity);

  switch (status) {
  case STAT_NOOP: return _exec_aline_body();
  case STAT_OK:
    ex.section = SECTION_BODY;
    ex.section_new = true;

    return STAT_EAGAIN;

  default: return status;
  }
}


static float _compute_next_segment_velocity() {
  if (ex.section_new) {
    if (ex.section == SECTION_HEAD) return mp_runtime_get_velocity();
    else return ex.cruise_velocity;
  }

  return ex.segment_velocity +
    (ex.section == SECTION_BODY ? 0 : mp_next_forward_diff(ex.forward_diff));
}


/// Replan current move to execute hold
///
/// Holds are initiated by the planner entering STATE_STOPPING.  In which case
/// _plan_hold() is called to replan the current move towards zero.  If it is
/// unable to plan to zero in the remaining length of the current move it will
/// decelerate as much as possible and then wait for the next move.  Once it is
/// possible to plan to zero velocity in the current move the remaining length
/// is put into the run buffer, which is still allocated, and the run buffer
/// becomes the hold point.  The hold is left by a start request in state.c.  At
/// this point the remaining buffers, if any, are replanned from zero up to
/// speed.
static void _plan_hold() {
  mp_buffer_t *bf = mp_queue_get_head(); // working buffer pointer
  if (!bf) return; // Oops! nothing's running

  // Examine and process current buffer and compute length left for decel
  float available_length =
    get_axis_vector_length(ex.final_target, mp_runtime_get_position());
  // Compute next_segment velocity, velocity left to shed to brake to zero
  float braking_velocity = _compute_next_segment_velocity();
  // Distance to brake to zero from braking_velocity, bf is OK to use here
  float braking_length =
    mp_get_target_length(braking_velocity, 0, bf->recip_jerk);

  // Hack to prevent Case 2 moves for perfect-fit decels.  Happens when homing.
  if (available_length < braking_length && fp_ZERO(bf->exit_velocity))
    braking_length = available_length;

  // Replan to decelerate
  ex.section = SECTION_TAIL;
  ex.section_new = true;
  ex.cruise_velocity = braking_velocity;
  ex.hold_planned = true;

  // Avoid creating segments before or after the hold which are too small.
  if (fabs(available_length - braking_length) < HOLD_DECELERATION_TOLERANCE) {
    // Case 0: deceleration fits almost exactly
    ex.exit_velocity = 0;
    ex.tail_length = available_length;

  } else if (braking_length <= available_length) {
    // Case 1: deceleration fits entirely into the remaining length
    // Setup tail to perform the deceleration
    ex.exit_velocity = 0;
    ex.tail_length = braking_length;

    // Re-use bf to run the remaining block length
    bf->length = available_length - braking_length;
    bf->delta_vmax = mp_get_target_velocity(0, bf->length, bf);
    bf->entry_vmax = 0;
    bf->state = BUFFER_RESTART; // Restart the buffer when done

  } else {
    // Case 2: deceleration exceeds length remaining in buffer
    // Replan to minimum (but non-zero) exit velocity
    ex.tail_length = available_length;
    ex.exit_velocity =
      braking_velocity - mp_get_target_velocity(0, available_length, bf);
  }
}


/// Initializes move runtime with a new planner buffer
static stat_t _exec_aline_init(mp_buffer_t *bf) {
  // Remove zero length lines.  Short lines have already been removed.
  if (fp_ZERO(bf->length)) return STAT_NOOP;

  // Initialize move
  copy_vector(ex.unit, bf->unit);
  copy_vector(ex.final_target, bf->target);

  ex.head_length = bf->head_length;
  ex.body_length = bf->body_length;
  ex.tail_length = bf->tail_length;
  ex.entry_velocity = bf->entry_velocity;
  ex.cruise_velocity = bf->cruise_velocity;
  ex.exit_velocity = bf->exit_velocity;

  ex.section = SECTION_HEAD;
  ex.section_new = true;
  ex.hold_planned = false;

  // Generate waypoints for position correction at section ends.  This helps
  // negate floating point errors in the forward differencing code.
  for (int axis = 0; axis < AXES; axis++) {
    float position = mp_runtime_get_axis_position(axis);

    ex.waypoint[SECTION_HEAD][axis] = position + ex.unit[axis] * ex.head_length;
    ex.waypoint[SECTION_BODY][axis] = position +
      ex.unit[axis] * (ex.head_length + ex.body_length);
    ex.waypoint[SECTION_TAIL][axis] = ex.final_target[axis];
  }

  return STAT_OK;
}


/// Aline execution routines
///
/// Everything here fires from interrupts and must be interrupt safe
///
/// Returns:
///
///   STAT_OK        move is done
///   STAT_EAGAIN    move is not finished - has more segments to run
///   STAT_NOOP      cause no stepper operation - do not load the move
///   STAT_xxxxx     fatal error.  Ends the move and frees the bf buffer
///
/// This routine is called from the (LO) interrupt level.  The interrupt
/// sequencing relies on the correct behavior of these routines.
/// Each call to _exec_aline() must execute and prep *one and only one*
/// segment.  If the segment is not the last segment in the bf buffer the
/// _aline() returns STAT_EAGAIN. If it's the last segment it returns
/// STAT_OK.  If it encounters a fatal error that would terminate the move it
/// returns a valid error code.
///
/// Notes:
///
/// [1] Returning STAT_OK ends the move and frees the bf buffer.
///     Returning STAT_OK at does NOT advance position meaning
///     any position error will be compensated by the next move.
///
/// Operation:
///
/// Aline generates jerk-controlled S-curves as per Ed Red's course notes:
///
///   http://www.et.byu.edu/~ered/ME537/Notes/Ch5.pdf
///   http://www.scribd.com/doc/63521608/Ed-Red-Ch5-537-Jerk-Equations
///
/// A full trapezoid is divided into 5 periods.  Periods 1 and 2 are the
/// first and second halves of the acceleration ramp (the concave and convex
/// parts of the S curve in the "head").  Periods 3 and 4 are the first
/// and second parts of the deceleration ramp (the tail).  There is also
/// a period for the constant-velocity plateau of the trapezoid (the body).
/// There are many possible degenerate trapezoids where any of the 5 periods
/// may be zero length but note that either none or both of a ramping pair can
/// be zero.
///
/// The equations that govern the acceleration and deceleration ramps are:
///
///   Period 1    V = Vi + Jm * (T^2) / 2
///   Period 2    V = Vh + As * T - Jm * (T^2) / 2
///   Period 3    V = Vi - Jm * (T^2) / 2
///   Period 4    V = Vh + As * T + Jm * (T^2) / 2
///
/// move_time is the actual time of the move, accel_time is the time value
/// needed to compute the velocity taking the initial velocity into account.
/// move_time does not need to.
stat_t mp_exec_aline(mp_buffer_t *bf) {
  stat_t status = STAT_OK;

  // Start a new move
  if (bf->state == BUFFER_INIT) {
    bf->state = BUFFER_ACTIVE;
    status = _exec_aline_init(bf);
    if (status != STAT_OK) return status;
  }

  // Plan holds
  if (mp_get_state() == STATE_STOPPING && !ex.hold_planned) _plan_hold();

  // Main segment dispatch
  switch (ex.section) {
  case SECTION_HEAD: status = _exec_aline_head(); break;
  case SECTION_BODY: status = _exec_aline_body(); break;
  case SECTION_TAIL: status = _exec_aline_tail(); break;
  }

  // Set runtime velocity on exit
  if (status != STAT_EAGAIN) mp_runtime_set_velocity(ex.exit_velocity);

  return status;
}


/// Dequeues buffers, initializes them, executes their callbacks and cleans up.
///
/// This is the guts of the planner runtime execution.  Because this routine is
/// run in an interrupt the state changes must be carefully ordered.
stat_t mp_exec_move() {
  // Check if we can run a buffer
  mp_buffer_t *bf = mp_queue_get_head();
  if (mp_get_state() == STATE_ESTOPPED || mp_get_state() == STATE_HOLDING ||
      !bf) {
    mp_runtime_set_velocity(0);
    mp_runtime_set_busy(false);

    return STAT_NOOP; // Nothing running
  }

  // Process new buffers
  if (bf->state == BUFFER_NEW) {
    // On restart wait a bit to give planner queue a chance to fill
    if (!mp_runtime_is_busy() && mp_queue_get_fill() < PLANNER_EXEC_MIN_FILL &&
      !rtc_expired(bf->ts + PLANNER_EXEC_DELAY)) return STAT_NOOP;

    // Take control of buffer
    bf->state = BUFFER_INIT;
    bf->replannable = false;

    // Update runtime
    mp_runtime_set_line(bf->line);
  }

  // Execute the buffer
  stat_t status = bf->cb(bf);

  // Signal that we are busy only if a move was queued.  This means that
  // nonstop buffers, i.e. non-plan-to-zero commands, will not cause the
  // runtime to enter the busy state.  This causes mp_exec_move() to continue
  // to wait above for the planner buffer to fill when a new stream starts
  // with some nonstop buffers.  If this weren't so, the code below
  // which marks the next buffer not replannable would lock the first move
  // buffer and cause it to be unnecessarily planned to zero.
  if (status == STAT_EAGAIN || status == STAT_OK) mp_runtime_set_busy(true);

  // Process finished buffers
  if (status != STAT_EAGAIN) {
    // Signal that we've encountered a stopping point
    if (fp_ZERO(mp_runtime_get_velocity()) &&
        (mp_get_state() == STATE_STOPPING || bf->hold)) mp_state_holding();

    // Handle buffer restarts and deallocation
    if (bf->state == BUFFER_RESTART) bf->state = BUFFER_NEW;
    else {
      // Solves a potential race condition where the current buffer ends but
      // the new buffer has not started because the current one is still
      // being run by the steppers.  Planning can overwrite the new buffer.
      // See notes above.
      mp_buffer_next(bf)->replannable = false;

      mp_queue_pop(); // Release buffer

      // Enter READY state
      if (mp_queue_is_empty()) mp_state_idle();
    }
  }

  // Convert return status for stepper.c
  switch (status) {
  case STAT_NOOP:
    // Tell caller to call again if there is more in the queue
    return mp_queue_is_empty() ? STAT_NOOP : STAT_EAGAIN;
  case STAT_EAGAIN: return STAT_OK;   // A move was queued, call again later
  default: return status;
  }
}
