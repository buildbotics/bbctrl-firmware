/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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

#include "machine.h"
#include "switch.h"
#include "util.h"
#include "report.h"

#include "plan/planner.h"

#include <avr/pgmspace.h>

#include <stdbool.h>
#include <math.h>
#include <stdio.h>


typedef void (*homing_func_t)(int8_t axis);

static void _homing_axis_start(int8_t axis);


/// persistent homing runtime variables
struct hmHomingSingleton {
  // controls for homing cycle
  int8_t axis;                    // axis currently being homed

  /// homing switch for current axis (index into switch flag table)
  int8_t homing_switch;
  int8_t limit_switch;            // limit switch for current axis or -1 if none

  uint8_t homing_closed;          // 0=open, 1=closed
  uint8_t limit_closed;           // 0=open, 1=closed
  /// G28.4 flag. true = set coords to zero at the end of homing cycle
  uint8_t set_coordinates;
  homing_func_t func;             // binding for callback function state machine

  // per-axis parameters
  /// set to 1 for positive (max), -1 for negative (to min);
  float direction;
  float search_travel;            // signed distance to travel in search
  float search_velocity;          // search speed as positive number
  float latch_velocity;           // latch speed as positive number
  /// max distance to back off switch during latch phase
  float latch_backoff;
  /// distance to back off switch before setting zero
  float zero_backoff;
  /// maximum distance of switch clearing backoffs before erring out
  float max_clear_backoff;

  // state saved from gcode model
  uint8_t saved_units_mode;       // G20,G21 global setting
  uint8_t saved_coord_system;     // G54 - G59 setting
  uint8_t saved_distance_mode;    // G90,G91 global setting
  uint8_t saved_feed_rate_mode;   // G93,G94 global setting
  float saved_feed_rate;          // F setting
  float saved_jerk;               // saved and restored for each axis homed
};
static struct hmHomingSingleton hm;


// G28.2 homing cycle

/* Homing works from a G28.2 according to the following writeup:
 *
 *   https://github.com/synthetos
 *     /TinyG/wiki/Homing-and-Limits-Description-and-Operation
 *
 * How does this work?
 *
 * Homing is invoked using a G28.2 command with 1 or more axes specified in the
 * command: e.g. g28.2 x0 y0 z0  (FYI: the number after each axis is irrelevant)
 *
 * Homing is always run in the following order - for each enabled axis:
 *   Z,X,Y,A            Note: B and C cannot be homed
 *
 * At the start of a homing cycle those switches configured for homing
 * (or for homing and limits) are treated as homing switches (they are modal).
 *
 * After initialization the following sequence is run for each axis to be homed:
 *
 *   0. If a homing or limit switch is closed on invocation, clear the switch
 *   1. Drive towards the homing switch at search velocity until switch is hit
 *   2. Drive away from the homing switch at latch velocity until switch opens
 *   3. Back off switch by the zero backoff distance and set zero for that axis
 *
 * Homing works as a state machine that is driven by registering a callback
 * function at hm.func() for the next state to be run. Once the axis is
 * initialized each callback basically does two things (1) start the move
 * for the current function, and (2) register the next state with hm.func().
 * When a move is started it will either be interrupted if the homing switch
 * changes state, This will cause the move to stop with a feedhold. The other
 * thing that can happen is the move will run to its full length if no switch
 * change is detected (hit or open),
 *
 * Once all moves for an axis are complete the next axis in the sequence is
 * homed.
 *
 * When a homing cycle is initiated the homing state is set to HOMING_NOT_HOMED
 * When homing completes successfully this is set to HOMING_HOMED, otherwise it
 * remains HOMING_NOT_HOMED.
 */
/* --- Some further details ---
 *
 * Note: When coding a cycle (like this one) you get to perform one queued
 * move per entry into the continuation, then you must exit.
 *
 * Another Note: When coding a cycle (like this one) you must wait until
 * the last move has actually been queued (or has finished) before declaring
 * the cycle to be done. Otherwise there is a nasty race condition
 * that will accept the next command before the position of
 * the final move has been recorded in the Gcode model. That's what the call
 * to cm_isbusy() is about.
 */


/* Return next axis in sequence based on axis in arg
 *
 * Accepts "axis" arg as the current axis; or -1 to retrieve the first axis
 * Returns next axis based on "axis" argument and if that axis is flagged for
 * homing in the gf struct
 * Returns -1 when all axes have been processed
 * Returns -2 if no axes are specified (Gcode calling error)
 * Homes Z first, then the rest in sequence
 *
 * Isolating this function facilitates implementing more complex and
 * user-specified axis homing orders
 */
static int8_t _get_next_axis(int8_t axis) {
  switch (axis) {
  case     -1: if (fp_TRUE(cm.gf.target[AXIS_Z])) return AXIS_Z;
  case AXIS_Z: if (fp_TRUE(cm.gf.target[AXIS_X])) return AXIS_X;
  case AXIS_X: if (fp_TRUE(cm.gf.target[AXIS_Y])) return AXIS_Y;
  case AXIS_Y: if (fp_TRUE(cm.gf.target[AXIS_A])) return AXIS_A;
  case AXIS_A: if (fp_TRUE(cm.gf.target[AXIS_B])) return AXIS_B;
  case AXIS_B: if (fp_TRUE(cm.gf.target[AXIS_C])) return AXIS_C;
  }

  return axis == -1 ? -2 : -1; // error or done
}


/// Helper to finalize homing, third part of return to home
static void _homing_finalize_exit() {
  mp_flush_planner(); // should be stopped, but in case of switch closure

  // restore to work coordinate system
  cm_set_coord_system(hm.saved_coord_system);
  cm_set_units_mode(hm.saved_units_mode);
  cm_set_distance_mode(hm.saved_distance_mode);
  cm_set_feed_rate_mode(hm.saved_feed_rate_mode);
  cm.gm.feed_rate = hm.saved_feed_rate;
  cm_set_motion_mode(MOTION_MODE_CANCEL_MOTION_MODE);
  cm_cycle_end();
  cm.cycle_state = CYCLE_OFF;
}


static void _homing_error_exit(stat_t status) {
  _homing_finalize_exit();
  status_error(status);
}


/// helper that actually executes the above moves
static void _homing_axis_move(int8_t axis, float target, float velocity) {
  float vect[] = {};
  float flags[] = {};

  vect[axis] = target;
  flags[axis] = true;
  cm.gm.feed_rate = velocity;
  mp_flush_planner(); // don't use cm_request_queue_flush() here
  cm_request_cycle_start();

  stat_t status = cm_straight_feed(vect, flags);
  if (status) _homing_error_exit(status);
}


/// End homing cycle in progress
static void _homing_abort(int8_t axis) {
  cm_set_axis_jerk(axis, hm.saved_jerk); // restore the max jerk value

  // homing state remains HOMING_NOT_HOMED
  _homing_error_exit(STAT_HOMING_CYCLE_FAILED);

  report_request();
}


/// set zero and finish up
static void _homing_axis_set_zero(int8_t axis) {
  if (hm.set_coordinates) {
    cm_set_position(axis, 0);
    cm.homed[axis] = true;

  } else // do not set axis if in G28.4 cycle
    cm_set_position(axis, mp_get_runtime_work_position(axis));

  cm_set_axis_jerk(axis, hm.saved_jerk); // restore the max jerk value

  hm.func = _homing_axis_start;
}


/// backoff to zero position
static void _homing_axis_zero_backoff(int8_t axis) {
  _homing_axis_move(axis, hm.zero_backoff, hm.search_velocity);
  hm.func = _homing_axis_set_zero;
}


/// Slow reverse until switch opens again
static void _homing_axis_latch(int8_t axis) {
  // verify assumption that we arrived here because of homing switch closure
  // rather than user-initiated feedhold or other disruption
  if (!switch_is_active(hm.homing_switch)) hm.func = _homing_abort;

  else {
    _homing_axis_move(axis, hm.latch_backoff, hm.latch_velocity);
    hm.func = _homing_axis_zero_backoff;
  }
}


/// Fast search for switch, closes switch
static void _homing_axis_search(int8_t axis) {
  // use the homing jerk for search onward
  cm_set_axis_jerk(axis, cm.a[axis].jerk_homing);
  _homing_axis_move(axis, hm.search_travel, hm.search_velocity);
  hm.func = _homing_axis_latch;
}


/// Initiate a clear to move off a switch that is thrown at the start
static void _homing_axis_clear(int8_t axis) {
  // Handle an initial switch closure by backing off the closed switch
  // NOTE: Relies on independent switches per axis (not shared)

  if (switch_is_active(hm.homing_switch))
    _homing_axis_move(axis, hm.latch_backoff, hm.search_velocity);

  else if (switch_is_active(hm.limit_switch))
    _homing_axis_move(axis, -hm.latch_backoff, hm.search_velocity);

  hm.func = _homing_axis_search;
}


/// Get next axis, initialize variables, call the clear
static void _homing_axis_start(int8_t axis) {
  // get the first or next axis
  if ((axis = _get_next_axis(axis)) < 0) { // axes are done or error
    if (axis == -1) {                                    // -1 is done
      cm.homing_state = HOMING_HOMED;
      _homing_finalize_exit();
      return;

    } else if (axis == -2)                               // -2 is error
      return _homing_error_exit(STAT_HOMING_ERROR_BAD_OR_NO_AXIS);
  }

  // clear the homed flag for axis to move w/o triggering soft limits
  cm.homed[axis] = false;

  // trap axis mis-configurations
  if (fp_ZERO(cm.a[axis].search_velocity))
    return _homing_error_exit(STAT_HOMING_ERROR_ZERO_SEARCH_VELOCITY);
  if (fp_ZERO(cm.a[axis].latch_velocity))
    return _homing_error_exit(STAT_HOMING_ERROR_ZERO_LATCH_VELOCITY);
  if (cm.a[axis].latch_backoff < 0)
    return _homing_error_exit(STAT_HOMING_ERROR_NEGATIVE_LATCH_BACKOFF);

  // calculate and test travel distance
  float travel_distance =
    fabs(cm.a[axis].travel_max - cm.a[axis].travel_min) +
    cm.a[axis].latch_backoff;
  if (fp_ZERO(travel_distance))
    return _homing_error_exit(STAT_HOMING_ERROR_TRAVEL_MIN_MAX_IDENTICAL);

  hm.axis = axis; // persist the axis
  // search velocity is always positive
  hm.search_velocity = fabs(cm.a[axis].search_velocity);
  // latch velocity is always positive
  hm.latch_velocity = fabs(cm.a[axis].latch_velocity);

  // determine which switch to use
  bool min_enabled = switch_is_enabled(MIN_SWITCH(axis));
  bool max_enabled = switch_is_enabled(MAX_SWITCH(axis));

  if (min_enabled) {
    // setup parameters homing to the minimum switch
    hm.homing_switch = MIN_SWITCH(axis);         // min is the homing switch
    hm.limit_switch = MAX_SWITCH(axis);          // max would be limit switch
    hm.search_travel = -travel_distance;         // in negative direction
    hm.latch_backoff = cm.a[axis].latch_backoff; // in positive direction
    hm.zero_backoff = cm.a[axis].zero_backoff;

  } else if (max_enabled) {
    // setup parameters for positive travel (homing to the maximum switch)
    hm.homing_switch = MAX_SWITCH(axis);          // max is homing switch
    hm.limit_switch = MIN_SWITCH(axis);           // min would be limit switch
    hm.search_travel = travel_distance;           // in positive direction
    hm.latch_backoff = -cm.a[axis].latch_backoff; // in negative direction
    hm.zero_backoff = -cm.a[axis].zero_backoff;

  } else {
    // if homing is disabled for the axis then skip to the next axis
    hm.limit_switch = -1; // disable the limit switch parameter
    hm.func = _homing_axis_start;
    return;
  }

  hm.saved_jerk = cm_get_axis_jerk(axis); // save the max jerk value
  hm.func = _homing_axis_clear; // start the clear
}


bool cm_is_homing() {
  return cm.cycle_state == CYCLE_HOMING;
}


/// G28.2 homing cycle using limit switches
void cm_homing_cycle_start() {
  // save relevant non-axis parameters from Gcode model
  hm.saved_units_mode = cm_get_units_mode(&cm.gm);
  hm.saved_coord_system = cm_get_coord_system(&cm.gm);
  hm.saved_distance_mode = cm_get_distance_mode(&cm.gm);
  hm.saved_feed_rate_mode = cm_get_feed_rate_mode(&cm.gm);
  hm.saved_feed_rate = cm_get_feed_rate(&cm.gm);

  // set working values
  cm_set_units_mode(MILLIMETERS);
  cm_set_distance_mode(INCREMENTAL_MODE);
  cm_set_coord_system(ABSOLUTE_COORDS);    // in machine coordinates
  cm_set_feed_rate_mode(UNITS_PER_MINUTE_MODE);
  hm.set_coordinates = true;

  hm.axis = -1;                            // set to retrieve initial axis
  hm.func = _homing_axis_start;            // bind initial processing function
  cm.cycle_state = CYCLE_HOMING;
  cm.homing_state = HOMING_NOT_HOMED;
}


void cm_homing_cycle_start_no_set() {
  cm_homing_cycle_start();
  hm.set_coordinates = false; // don't update position variables at end of cycle
}


/// Main loop callback for running the homing cycle
void cm_homing_callback() {
  if (cm.cycle_state != CYCLE_HOMING || cm_get_runtime_busy()) return;
  hm.func(hm.axis); // execute the current homing move
}
