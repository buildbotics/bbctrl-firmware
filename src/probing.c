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

#include "machine.h"
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
  void (*func)();                 // binding for callback function state machine

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


/* Note: When coding a cycle (like this one) you get to perform one
 * queued move per entry into the continuation, then you must exit.
 *
 * Another Note: When coding a cycle (like this one) you must wait
 * until the last move has actually been queued (or has finished)
 * before declaring the cycle to be done. Otherwise there is a nasty
 * race condition in the tg_controller() that will accept the next
 * command before the position of the final move has been recorded in
 * the Gcode model. That's what the call to mach_get_runtime_busy() is
 * about.
 */


static void _probe_restore_settings() {
  // we should be stopped now, but in case of switch closure
  mp_flush_planner();

  // restore axis jerk
  for (int axis = 0; axis < AXES; axis++ )
    mach_set_axis_jerk(axis, pb.saved_jerk[axis]);

  // restore coordinate system and distance mode
  mach_set_coord_system(pb.saved_coord_system);
  mach_set_distance_mode(pb.saved_distance_mode);

  // update the model with actual position
  mach_set_motion_mode(MOTION_MODE_CANCEL_MOTION_MODE);
  mach_cycle_end();
  mach_set_cycle(CYCLE_MACHINING);
}


static void _probing_error_exit(stat_t status) {
  _probe_restore_settings(); // clean up and exit
  status_error(status);
}


static void _probing_finish() {
  bool closed = switch_is_active(SW_PROBE);
  mach.probe_state = closed ? PROBE_SUCCEEDED : PROBE_FAILED;

  for (int axis = 0; axis < AXES; axis++) {
    // if we got here because of a feed hold keep the model position correct
    mach_set_position(axis, mp_get_runtime_work_position(axis));

    // store the probe results
    mach.probe_results[axis] = mach_get_absolute_position(axis);
  }

  _probe_restore_settings();
}


static void _probing_start() {
  // initial probe state, don't probe if we're already contacted!
  bool closed = switch_is_active(SW_PROBE);

  if (!closed) {
    stat_t status = mach_straight_feed(pb.target, pb.flags);
    if (status) return _probing_error_exit(status);
  }

  pb.func = _probing_finish;
}


/* G38.2 homing cycle using limit switches
 *
 * These initializations are required before starting the probing cycle.
 * They must be done after the planner has exhasted all current CYCLE moves as
 * they affect the runtime (specifically the switch modes). Side effects would
 * include limit switches initiating probe actions instead of just killing
 * movement
 */
static void _probing_init() {
  // NOTE: it is *not* an error condition for the probe not to trigger.
  // it is an error for the limit or homing switches to fire, or for some other
  // configuration error.
  mach.probe_state = PROBE_FAILED;
  mach_set_cycle(CYCLE_PROBING);

  // initialize the axes - save the jerk settings & switch to the jerk_homing
  // settings
  for (int axis = 0; axis < AXES; axis++) {
    pb.saved_jerk[axis] = mach_get_axis_jerk(axis);   // save the max jerk value
    // use homing jerk for probe
    mach_set_axis_jerk(axis, mach.a[axis].jerk_homing);
    pb.start_position[axis] = mach_get_absolute_position(axis);
  }

  // error if the probe target is too close to the current position
  if (get_axis_vector_length(pb.start_position, pb.target) <
      MINIMUM_PROBE_TRAVEL)
    _probing_error_exit(STAT_PROBE_INVALID_DEST);

  // error if the probe target requires a move along the A/B/C axes
  for (int axis = 0; axis < AXES; axis++)
    if (fp_NE(pb.start_position[axis], pb.target[axis]))
      _probing_error_exit(STAT_MOVE_DURING_PROBE);

  // probe in absolute machine coords
  pb.saved_coord_system = mach_get_coord_system(&mach.gm);
  pb.saved_distance_mode = mach_get_distance_mode(&mach.gm);
  mach_set_distance_mode(ABSOLUTE_MODE);
  mach_set_coord_system(ABSOLUTE_COORDS);

  mach_spindle_control(SPINDLE_OFF);

  // start the move
  pb.func = _probing_start;
}


bool mach_is_probing() {
  return mach_get_cycle() == CYCLE_PROBING || mach.probe_state == PROBE_WAITING;
}


/// G38.2 homing cycle using limit switches
stat_t mach_straight_probe(float target[], float flags[]) {
  // trap zero feed rate condition
  if (mach.gm.feed_rate_mode != INVERSE_TIME_MODE && fp_ZERO(mach.gm.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  // trap no axes specified
  if (fp_NOT_ZERO(flags[AXIS_X]) && fp_NOT_ZERO(flags[AXIS_Y]) &&
      fp_NOT_ZERO(flags[AXIS_Z]))
    return STAT_GCODE_AXIS_IS_MISSING;

  // set probe move endpoint
  copy_vector(pb.target, target);      // set probe move endpoint
  copy_vector(pb.flags, flags);        // set axes involved on the move
  clear_vector(mach.probe_results);      // clear the old probe position.
  // NOTE: relying on probe_results will not detect a probe to (0, 0, 0).

  // wait until planner queue empties before completing initialization
  mach.probe_state = PROBE_WAITING;
  pb.func = _probing_init;             // bind probing initialization function

  return STAT_OK;
}


/// main loop callback for running the homing cycle
void mach_probe_callback() {
  // sync to planner move ends
  if (!mach_is_probing() || mach_get_runtime_busy()) return;

  pb.func(); // execute the current homing move
}
