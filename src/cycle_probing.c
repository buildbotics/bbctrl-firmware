/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S Hart, Jr.
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

#include "canonical_machine.h"
#include "spindle.h"
#include "switch.h"
#include "util.h"

#include "plan/planner.h"

#include <avr/pgmspace.h>

#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>


#define MINIMUM_PROBE_TRAVEL 0.254


struct pbProbingSingleton {       // persistent probing runtime variables
  stat_t (*func)();               // binding for callback function state machine

  // switch configuration
  uint8_t probe_switch;           // which switch should we check?
  uint8_t saved_switch_type;      // saved switch type NO/NC
  uint8_t saved_switch_mode;      // save the probe switch's original settings

  // state saved from gcode model
  uint8_t saved_distance_mode;    // G90,G91 global setting
  uint8_t saved_coord_system;     // G54 - G59 setting
  float saved_jerk[AXES];         // saved and restored for each axis

  // probe destination
  float start_position[AXES];
  float target[AXES];
  float flags[AXES];
};

static struct pbProbingSingleton pb;


static stat_t _probing_init();
static stat_t _probing_start();
static stat_t _probing_finish();
static stat_t _probing_finalize_exit();
static stat_t _probing_error_exit(int8_t axis);


/// A convenience for setting the next dispatch vector and exiting
uint8_t _set_pb_func(uint8_t (*func)()) {
  pb.func = func;
  return STAT_EAGAIN;
}


/* All cm_probe_cycle_start does is prevent any new commands from
 * queueing to the planner so that the planner can move to a sop and
 * report MACHINE_PROGRAM_STOP.  OK, it also queues the function
 * that's called once motion has stopped.
 *
 * Note: When coding a cycle (like this one) you get to perform one
 * queued move per entry into the continuation, then you must exit.
 *
 * Another Note: When coding a cycle (like this one) you must wait
 * until the last move has actually been queued (or has finished)
 * before declaring the cycle to be done. Otherwise there is a nasty
 * race condition in the tg_controller() that will accept the next
 * command before the position of the final move has been recorded in
 * the Gcode model. That's what the call to cm_get_runtime_busy() is
 * about.
 */


/// G38.2 homing cycle using limit switches
uint8_t cm_straight_probe(float target[], float flags[]) {
  // trap zero feed rate condition
  if (cm.gm.feed_rate_mode != INVERSE_TIME_MODE && fp_ZERO(cm.gm.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  // trap no axes specified
  if (fp_NOT_ZERO(flags[AXIS_X]) && fp_NOT_ZERO(flags[AXIS_Y]) &&
      fp_NOT_ZERO(flags[AXIS_Z]))
    return STAT_GCODE_AXIS_IS_MISSING;

  // set probe move endpoint
  copy_vector(pb.target, target);      // set probe move endpoint
  copy_vector(pb.flags, flags);        // set axes involved on the move
  clear_vector(cm.probe_results);      // clear the old probe position.
  // NOTE: relying on probe_result will not detect a probe to 0,0,0.

  // wait until planner queue empties before completing initialization
  cm.probe_state = PROBE_WAITING;
  pb.func = _probing_init;             // bind probing initialization function
  return STAT_OK;
}


/// main loop callback for running the homing cycle
uint8_t cm_probe_callback() {
  if (cm.cycle_state != CYCLE_PROBE && cm.probe_state != PROBE_WAITING)
    return STAT_NOOP; // exit if not in a probe cycle or waiting for one

  if (cm_get_runtime_busy()) return STAT_EAGAIN;    // sync to planner move ends
  return pb.func(); // execute the current homing move
}

/* G38.2 homing cycle using limit switches
 *
 * These initializations are required before starting the probing cycle.
 * They must be done after the planner has exhasted all current CYCLE moves as
 * they affect the runtime (specifically the switch modes). Side effects would
 * include limit switches initiating probe actions instead of just killing
 * movement
 */
static uint8_t _probing_init() {
  // NOTE: it is *not* an error condition for the probe not to trigger.
  // it is an error for the limit or homing switches to fire, or for some other
  // configuration error.
  cm.probe_state = PROBE_FAILED;
  cm.cycle_state = CYCLE_PROBE;

  // initialize the axes - save the jerk settings & switch to the jerk_homing
  // settings
  for (uint8_t axis = 0; axis < AXES; axis++) {
    pb.saved_jerk[axis] = cm_get_axis_jerk(axis);   // save the max jerk value
    cm_set_axis_jerk(axis, cm.a[axis].jerk_homing); // use homing jerk for probe
    pb.start_position[axis] = cm_get_absolute_position(axis);
  }

  // error if the probe target is too close to the current position
  if (get_axis_vector_length(pb.start_position, pb.target) <
      MINIMUM_PROBE_TRAVEL)
    _probing_error_exit(-2);

  // error if the probe target requires a move along the A/B/C axes
  for (uint8_t axis = 0; axis < AXES; axis++)
    if (fp_NE(pb.start_position[axis], pb.target[axis]))
      _probing_error_exit(axis);

  // initialize the probe switch

  // switch the switch type mode for the probe
  // FIXME: we should be able to use the homing switch at this point too,
  // Can't because switch mode is global and our probe is NO, not NC.

  pb.probe_switch = SW_MIN_Z; // FIXME: hardcoded...
  pb.saved_switch_mode = switch_get_mode(pb.probe_switch);

  switch_set_mode(pb.probe_switch, SW_MODE_HOMING);
  // save the switch type for recovery later.
  pb.saved_switch_type = switch_get_type(pb.probe_switch);
  // contact probes are NO switches... usually
  switch_set_type(pb.probe_switch, SW_TYPE_NORMALLY_OPEN);
  // re-init to pick up new switch settings
  switch_init();

  // probe in absolute machine coords
  pb.saved_coord_system = cm_get_coord_system(&cm.gm);
  pb.saved_distance_mode = cm_get_distance_mode(&cm.gm);
  cm_set_distance_mode(ABSOLUTE_MODE);
  cm_set_coord_system(ABSOLUTE_COORDS);

  cm_spindle_control(SPINDLE_OFF);

  // start the move
  return _set_pb_func(_probing_start);
}


static stat_t _probing_start() {
  // initial probe state, don't probe if we're already contacted!
  bool closed = switch_get_active(pb.probe_switch);

  if (!closed) RITORNO(cm_straight_feed(pb.target, pb.flags));

  return _set_pb_func(_probing_finish);
}


static stat_t _probing_finish() {
  bool closed = switch_get_active(pb.probe_switch);
  cm.probe_state = closed ? PROBE_SUCCEEDED : PROBE_FAILED;

  for (uint8_t axis = 0; axis < AXES; axis++) {
    // if we got here because of a feed hold keep the model position correct
    cm_set_position(axis, mp_get_runtime_work_position(axis));

    // store the probe results
    cm.probe_results[axis] = cm_get_absolute_position(axis);
  }

  return _set_pb_func(_probing_finalize_exit);
}


static void _probe_restore_settings() {
  // we should be stopped now, but in case of switch closure
  mp_flush_planner();

  switch_set_type(pb.probe_switch, pb.saved_switch_type);
  switch_set_mode(pb.probe_switch, pb.saved_switch_mode);
  switch_init(); // re-init to pick up changes

  // restore axis jerk
  for (uint8_t axis = 0; axis < AXES; axis++ )
    cm_set_axis_jerk(axis, pb.saved_jerk[axis]);

  // restore coordinate system and distance mode
  cm_set_coord_system(pb.saved_coord_system);
  cm_set_distance_mode(pb.saved_distance_mode);

  // update the model with actual position
  cm_set_motion_mode(MOTION_MODE_CANCEL_MOTION_MODE);
  cm_cycle_end();
  cm.cycle_state = CYCLE_OFF;
}


static stat_t _probing_finalize_exit() {
  _probe_restore_settings();
  return STAT_OK;
}


static stat_t _probing_error_exit(int8_t axis) {
  // Generate the warning message.
  if (axis == -2)
    fprintf_P(stderr, PSTR("Probing error - invalid probe destination"));
  else fprintf_P(stderr,
                 PSTR("Probing error - %c axis cannot move during probing"),
                 cm_get_axis_char(axis));

  // clean up and exit
  _probe_restore_settings();

  return STAT_PROBE_CYCLE_FAILED;
}
