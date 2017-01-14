/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "homing.h"

#include "axis.h"
#include "machine.h"
#include "switch.h"
#include "gcode_parser.h"
#include "report.h"

#include "plan/planner.h"
#include "plan/runtime.h"
#include "plan/state.h"


typedef void (*homing_func_t)(int8_t axis);

static void _homing_axis_start(int8_t axis);


typedef enum {
  HOMING_NOT_HOMED,     // machine is not homed
  HOMING_HOMED,         // machine is homed
  HOMING_WAITING,       // machine waiting to be homed
} homing_state_t;


/// persistent homing runtime variables
typedef struct {
  homing_state_t state;           // homing cycle sub-state machine
  bool homed[AXES];               // individual axis homing flags

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
  uint8_t saved_units;            // G20,G21 global setting
  uint8_t saved_coord_system;     // G54 - G59 setting
  uint8_t saved_distance_mode;    // G90,G91 global setting
  uint8_t saved_feed_mode;        // G93,G94 global setting
  float saved_feed_rate;          // F setting
  float saved_jerk;               // saved and restored for each axis homed
} homing_t;


static homing_t hm = {0};


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
 *
 * Notes:
 *
 *   1. When coding a cycle (like this one) you get to perform one queued
 *      move per entry into the continuation, then you must exit.
 *
 *   2. When coding a cycle (like this one) you must wait until
 *      the last move has actually been queued (or has finished) before
 *      declaring the cycle to be done. Otherwise there is a nasty race
 *      condition that will accept the next command before the position of
 *      the final move has been recorded in the Gcode model.
 */


/*** Return next axis in sequence based on axis in arg
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
  case     -1: if (parser.gf.target[AXIS_Z]) return AXIS_Z;
  case AXIS_Z: if (parser.gf.target[AXIS_X]) return AXIS_X;
  case AXIS_X: if (parser.gf.target[AXIS_Y]) return AXIS_Y;
  case AXIS_Y: if (parser.gf.target[AXIS_A]) return AXIS_A;
  case AXIS_A: if (parser.gf.target[AXIS_B]) return AXIS_B;
  case AXIS_B: if (parser.gf.target[AXIS_C]) return AXIS_C;
  }

  return axis == -1 ? -2 : -1; // error or done
}


/// Helper to finalize homing, third part of return to home
static void _homing_finalize_exit() {
  mp_flush_planner(); // should be stopped, but in case of switch closure

  // Restore saved machine state
  mach_set_coord_system(hm.saved_coord_system);
  mach_set_units(hm.saved_units);
  mach_set_distance_mode(hm.saved_distance_mode);
  mach_set_feed_mode(hm.saved_feed_mode);
  mach_set_feed_rate(hm.saved_feed_rate);
  mach_set_motion_mode(MOTION_MODE_CANCEL_MOTION_MODE);

  mp_set_cycle(CYCLE_MACHINING); // Default cycle
}


static void _homing_error_exit(stat_t status) {
  _homing_finalize_exit();
  status_error(status);
}


/// Execute moves
static void _homing_axis_move(int8_t axis, float target, float velocity) {
  float vect[AXES] = {0};
  bool flags[AXES] = {0};

  vect[axis] = target;
  flags[axis] = true;
  mach_set_feed_rate(velocity);
  mp_flush_planner(); // don't use mp_request_flush() here

  stat_t status = mach_feed(vect, flags);
  if (status) _homing_error_exit(status);
}


/// End homing cycle in progress
static void _homing_abort(int8_t axis) {
  axis_set_jerk_max(axis, hm.saved_jerk); // restore the max jerk value

  // homing state remains HOMING_NOT_HOMED
  _homing_error_exit(STAT_HOMING_CYCLE_FAILED);

  report_request();
}


/// set zero and finish up
static void _homing_axis_set_zero(int8_t axis) {
  if (hm.set_coordinates) {
    mach_set_axis_position(axis, 0);
    hm.homed[axis] = true;

  } else // do not set axis if in G28.4 cycle
    mach_set_axis_position(axis, mp_runtime_get_work_position(axis));

  axis_set_jerk_max(axis, hm.saved_jerk); // restore the max jerk value

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
      hm.state = HOMING_HOMED;
      _homing_finalize_exit();
      return;

    } else if (axis == -2)                               // -2 is error
      return _homing_error_exit(STAT_HOMING_ERROR_BAD_OR_NO_AXIS);
  }

  // clear the homed flag for axis to move w/o triggering soft limits
  hm.homed[axis] = false;

  // trap axis mis-configurations
  if (fp_ZERO(axis_get_search_velocity(axis)))
    return _homing_error_exit(STAT_HOMING_ERROR_ZERO_SEARCH_VELOCITY);
  if (fp_ZERO(axis_get_latch_velocity(axis)))
    return _homing_error_exit(STAT_HOMING_ERROR_ZERO_LATCH_VELOCITY);
  if (axis_get_latch_backoff(axis) < 0)
    return _homing_error_exit(STAT_HOMING_ERROR_NEGATIVE_LATCH_BACKOFF);

  // calculate and test travel distance
  float travel_distance =
    fabs(axis_get_travel_max(axis) - axis_get_travel_min(axis)) +
    axis_get_latch_backoff(axis);
  if (fp_ZERO(travel_distance))
    return _homing_error_exit(STAT_HOMING_ERROR_TRAVEL_MIN_MAX_IDENTICAL);

  hm.axis = axis; // persist the axis
  // search velocity is always positive
  hm.search_velocity = fabs(axis_get_search_velocity(axis));
  // latch velocity is always positive
  hm.latch_velocity = fabs(axis_get_latch_velocity(axis));

  // determine which switch to use
  bool min_enabled = switch_is_enabled(MIN_SWITCH(axis));
  bool max_enabled = switch_is_enabled(MAX_SWITCH(axis));

  if (min_enabled) {
    // setup parameters homing to the minimum switch
    hm.homing_switch = MIN_SWITCH(axis);         // min is the homing switch
    hm.limit_switch = MAX_SWITCH(axis);          // max would be limit switch
    hm.search_travel = -travel_distance;         // in negative direction
    hm.latch_backoff = axis_get_latch_backoff(axis); // in positive direction
    hm.zero_backoff = axis_get_zero_backoff(axis);

  } else if (max_enabled) {
    // setup parameters for positive travel (homing to the maximum switch)
    hm.homing_switch = MAX_SWITCH(axis);          // max is homing switch
    hm.limit_switch = MIN_SWITCH(axis);           // min would be limit switch
    hm.search_travel = travel_distance;           // in positive direction
    hm.latch_backoff = -axis_get_latch_backoff(axis); // in negative direction
    hm.zero_backoff = -axis_get_zero_backoff(axis);

  } else {
    // if homing is disabled for the axis then skip to the next axis
    hm.limit_switch = -1; // disable the limit switch parameter
    hm.func = _homing_axis_start;
    return;
  }

  hm.saved_jerk = axis_get_jerk_max(axis); // save the max jerk value
  hm.func = _homing_axis_clear; // start the clear
}


bool mach_is_homing() {
  return mp_get_cycle() == CYCLE_HOMING;
}


void mach_set_not_homed() {
  for (int axis = 0; axis < AXES; axis++)
    mach_set_homed(axis, false);
}


bool mach_get_homed(int axis) {
  return hm.homed[axis];
}


void mach_set_homed(int axis, bool homed) {
  // TODO save homed to EEPROM
  hm.homed[axis] = homed;
}


/// G28.2 homing cycle using limit switches
void mach_homing_cycle_start() {
  // save relevant non-axis parameters from Gcode model
  hm.saved_units = mach_get_units();
  hm.saved_coord_system = mach_get_coord_system();
  hm.saved_distance_mode = mach_get_distance_mode();
  hm.saved_feed_mode = mach_get_feed_mode();
  hm.saved_feed_rate = mach_get_feed_rate();

  // set working values
  mach_set_units(MILLIMETERS);
  mach_set_distance_mode(INCREMENTAL_MODE);
  mach_set_coord_system(ABSOLUTE_COORDS);  // in machine coordinates
  mach_set_feed_mode(UNITS_PER_MINUTE_MODE);
  hm.set_coordinates = true;

  hm.axis = -1;                            // set to retrieve initial axis
  hm.func = _homing_axis_start;            // bind initial processing function
  mp_set_cycle(CYCLE_HOMING);
  hm.state = HOMING_NOT_HOMED;
}


void mach_homing_cycle_start_no_set() {
  mach_homing_cycle_start();
  hm.set_coordinates = false; // don't update position variables at end of cycle
}


/// Main loop callback for running the homing cycle
void mach_homing_callback() {
  if (mp_get_cycle() != CYCLE_HOMING || mp_get_state() != STATE_READY) return;
  hm.func(hm.axis); // execute the current homing move
}
