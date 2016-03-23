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

/* This code is a loose implementation of Kramer, Proctor and Messina's
 * canonical machining functions as described in the NIST RS274/NGC v3
 *
 * The canonical machine is the layer between the Gcode parser and
 * the motion control code for a specific robot. It keeps state and
 * executes commands - passing the stateless commands to the motion
 * planning layer.
 *
 * System state contexts - Gcode models
 *
 *   Useful reference for doing C callbacks
 *   http://www.newty.de/fpt/fpt.html
 *
 *   There are 3 temporal contexts for system state: - The gcode
 *   model in the canonical machine (the MODEL context, held in gm) -
 *   The gcode model used by the planner (PLANNER context, held in
 *   bf's and mm) - The gcode model used during motion for reporting
 *   (RUNTIME context, held in mr)
 *
 *   It's a bit more complicated than this. The 'gm' struct contains
 *   the core Gcode model context. This originates in the canonical
 *   machine and is copied to each planner buffer (bf buffer) during
 *   motion planning. Finally, the gm context is passed to the
 *   runtime (mr) for the RUNTIME context. So at last count the Gcode
 *   model exists in as many as 30 copies in the system. (1+28+1)
 *
 *   Depending on the need, any one of these contexts may be called
 *   for reporting or by a function. Most typically, all new commends
 *   from the gcode parser work form the MODEL context, and status
 *   reports pull from the RUNTIME while in motion, and from MODEL
 *   when at rest. A convenience is provided in the ACTIVE_MODEL
 *   pointer to point to the right context.
 *
 * Synchronizing command execution
 *
 *   Some gcode commands only set the MODEL state for interpretation
 *   of the current Gcode block. For example,
 *   cm_set_feed_rate(). This sets the MODEL so the move time is
 *   properly calculated for the current (and subsequent) blocks, so
 *   it's effected immediately.
 *
 *   "Synchronous commands" are commands that affect the runtime need
 *   to be synchronized with movement. Examples include G4 dwells,
 *   program stops and ends, and most M commands.  These are queued
 *   into the planner queue and execute from the queue. Synchronous
 *   commands work like this:
 *
 *     - Call the cm_xxx_xxx() function which will do any input
 *       validation and return an error if it detects one.
 *
 *     - The cm_ function calls mp_queue_command(). Arguments are a
 *       callback to the _exec_...() function, which is the runtime
 *       execution routine, and any arguments that are needed by the
 *       runtime. See typedef for *exec in planner.h for details
 *
 *     - mp_queue_command() stores the callback and the args in a
          planner buffer.
 *
 *     - When planner execution reaches the buffer it executes the
 *       callback w/ the args.  Take careful note that the callback
 *       executes under an interrupt, so beware of variables that may
 *       need to be volatile.
 *
 *   Note: - The synchronous command execution mechanism uses 2
 *   vectors in the bf buffer to store and return values for the
 *   callback. It's obvious, but impractical to pass the entire bf
 *   buffer to the callback as some of these commands are actually
 *   executed locally and have no buffer.
 */

#include "canonical_machine.h"

#include "config.h"
#include "stepper.h"
#include "encoder.h"
#include "spindle.h"
#include "gpio.h"
#include "switch.h"
#include "hardware.h"
#include "util.h"
#include "usart.h"            // for serial queue flush

#include "plan/planner.h"
#include "plan/buffer.h"
#include "plan/feedhold.h"
#include "plan/dwell.h"
#include "plan/command.h"
#include "plan/arc.h"
#include "plan/line.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdio.h>


cmSingleton_t cm = {
  .junction_acceleration = JUNCTION_ACCELERATION,
  .chordal_tolerance = CHORDAL_TOLERANCE,
  .soft_limit_enable = SOFT_LIMIT_ENABLE,

  .arc_segment_len = ARC_SEGMENT_LENGTH,

  .coord_system = GCODE_DEFAULT_COORD_SYSTEM,
  .select_plane = GCODE_DEFAULT_PLANE,
  .units_mode = GCODE_DEFAULT_UNITS,
  .path_control = GCODE_DEFAULT_PATH_CONTROL,
  .distance_mode = GCODE_DEFAULT_DISTANCE_MODE,

  // Offsets
  .offset = {
    {}, // ABSOLUTE_COORDS

    {G54_X_OFFSET, G54_Y_OFFSET, G54_Z_OFFSET,
     G54_A_OFFSET, G54_B_OFFSET, G54_C_OFFSET},

    {G55_X_OFFSET, G55_Y_OFFSET, G55_Z_OFFSET,
     G55_A_OFFSET, G55_B_OFFSET, G55_C_OFFSET},

    {G56_X_OFFSET, G56_Y_OFFSET, G56_Z_OFFSET,
     G56_A_OFFSET, G56_B_OFFSET, G56_C_OFFSET},

    {G57_X_OFFSET, G57_Y_OFFSET, G57_Z_OFFSET,
     G57_A_OFFSET, G57_B_OFFSET, G57_C_OFFSET},

    {G58_X_OFFSET, G58_Y_OFFSET, G58_Z_OFFSET,
     G58_A_OFFSET, G58_B_OFFSET, G58_C_OFFSET},

    {G59_X_OFFSET, G59_Y_OFFSET, G59_Z_OFFSET,
     G59_A_OFFSET, G59_B_OFFSET, G59_C_OFFSET},
  },

  // Axes
  .a = {
    {
      .axis_mode =         X_AXIS_MODE,
      .velocity_max =      X_VELOCITY_MAX,
      .feedrate_max =      X_FEEDRATE_MAX,
      .travel_min =        X_TRAVEL_MIN,
      .travel_max =        X_TRAVEL_MAX,
      .jerk_max =          X_JERK_MAX,
      .jerk_homing =       X_JERK_HOMING,
      .junction_dev =      X_JUNCTION_DEVIATION,
      .search_velocity =   X_SEARCH_VELOCITY,
      .latch_velocity =    X_LATCH_VELOCITY,
      .latch_backoff =     X_LATCH_BACKOFF,
      .zero_backoff =      X_ZERO_BACKOFF,
    }, {
      .axis_mode =         Y_AXIS_MODE,
      .velocity_max =      Y_VELOCITY_MAX,
      .feedrate_max =      Y_FEEDRATE_MAX,
      .travel_min =        Y_TRAVEL_MIN,
      .travel_max =        Y_TRAVEL_MAX,
      .jerk_max =          Y_JERK_MAX,
      .jerk_homing =       Y_JERK_HOMING,
      .junction_dev =      Y_JUNCTION_DEVIATION,
      .search_velocity =   Y_SEARCH_VELOCITY,
      .latch_velocity =    Y_LATCH_VELOCITY,
      .latch_backoff =     Y_LATCH_BACKOFF,
      .zero_backoff =      Y_ZERO_BACKOFF,
    }, {
      .axis_mode =         Z_AXIS_MODE,
      .velocity_max =      Z_VELOCITY_MAX,
      .feedrate_max =      Z_FEEDRATE_MAX,
      .travel_min =        Z_TRAVEL_MIN,
      .travel_max =        Z_TRAVEL_MAX,
      .jerk_max =          Z_JERK_MAX,
      .jerk_homing =       Z_JERK_HOMING,
      .junction_dev =      Z_JUNCTION_DEVIATION,
      .search_velocity =   Z_SEARCH_VELOCITY,
      .latch_velocity =    Z_LATCH_VELOCITY,
      .latch_backoff =     Z_LATCH_BACKOFF,
      .zero_backoff =      Z_ZERO_BACKOFF,
    }, {
      .axis_mode =         A_AXIS_MODE,
      .velocity_max =      A_VELOCITY_MAX,
      .feedrate_max =      A_FEEDRATE_MAX,
      .travel_min =        A_TRAVEL_MIN,
      .travel_max =        A_TRAVEL_MAX,
      .jerk_max =          A_JERK_MAX,
      .jerk_homing =       A_JERK_HOMING,
      .junction_dev =      A_JUNCTION_DEVIATION,
      .radius =            A_RADIUS,
      .search_velocity =   A_SEARCH_VELOCITY,
      .latch_velocity =    A_LATCH_VELOCITY,
      .latch_backoff =     A_LATCH_BACKOFF,
      .zero_backoff =      A_ZERO_BACKOFF,
    }, {
      .axis_mode =         B_AXIS_MODE,
      .velocity_max =      B_VELOCITY_MAX,
      .feedrate_max =      B_FEEDRATE_MAX,
      .travel_min =        B_TRAVEL_MIN,
      .travel_max =        B_TRAVEL_MAX,
      .jerk_max =          B_JERK_MAX,
      .junction_dev =      B_JUNCTION_DEVIATION,
      .radius =            B_RADIUS,
    }, {
      .axis_mode =         C_AXIS_MODE,
      .velocity_max =      C_VELOCITY_MAX,
      .feedrate_max =      C_FEEDRATE_MAX,
      .travel_min =        C_TRAVEL_MIN,
      .travel_max =        C_TRAVEL_MAX,
      .jerk_max =          C_JERK_MAX,
      .junction_dev =      C_JUNCTION_DEVIATION,
      .radius =            C_RADIUS,
    }
  },

  .combined_state = COMBINED_READY,
  .machine_state = MACHINE_READY,

  // State
  .gm = {.motion_mode = MOTION_MODE_CANCEL_MOTION_MODE},
  .gmx = {.block_delete_switch = true},
  .gn = {},
  .gf = {},
};


// Command execution callbacks from planner queue
static void _exec_offset(float *value, float *flag);
static void _exec_change_tool(float *value, float *flag);
static void _exec_select_tool(float *value, float *flag);
static void _exec_mist_coolant_control(float *value, float *flag);
static void _exec_flood_coolant_control(float *value, float *flag);
static void _exec_absolute_origin(float *value, float *flag);
static void _exec_program_finalize(float *value, float *flag);

// Canonical Machine State functions

/// Combines raw states into something a user might want to see
uint8_t cm_get_combined_state() {
  if (cm.cycle_state == CYCLE_OFF) cm.combined_state = cm.machine_state;
  else if (cm.cycle_state == CYCLE_PROBE) cm.combined_state = COMBINED_PROBE;
  else if (cm.cycle_state == CYCLE_HOMING) cm.combined_state = COMBINED_HOMING;
  else {
    if (cm.motion_state == MOTION_RUN) cm.combined_state = COMBINED_RUN;
    if (cm.motion_state == MOTION_HOLD) cm.combined_state = COMBINED_HOLD;
  }

  if (cm.machine_state == MACHINE_SHUTDOWN)
    cm.combined_state = COMBINED_SHUTDOWN;

  return cm.combined_state;
}

uint8_t cm_get_machine_state() {return cm.machine_state;}
uint8_t cm_get_cycle_state() {return cm.cycle_state;}
uint8_t cm_get_motion_state() {return cm.motion_state;}
uint8_t cm_get_hold_state() {return cm.hold_state;}
uint8_t cm_get_homing_state() {return cm.homing_state;}


/// Adjusts active model pointer as well
void cm_set_motion_state(uint8_t motion_state) {
  cm.motion_state = motion_state;

  switch (motion_state) {
  case (MOTION_STOP): ACTIVE_MODEL = MODEL; break;
  case (MOTION_RUN):  ACTIVE_MODEL = RUNTIME; break;
  case (MOTION_HOLD): ACTIVE_MODEL = RUNTIME; break;
  }
}


// These getters and setters will work on any gm model with inputs:
//   MODEL, PLANNER, RUNTIME, ACTIVE_MODEL
uint32_t cm_get_linenum(GCodeState_t *gcode_state) {
  return gcode_state->linenum;
}


uint8_t cm_get_motion_mode(GCodeState_t *gcode_state) {
  return gcode_state->motion_mode;
}


uint8_t cm_get_coord_system(GCodeState_t *gcode_state) {
  return gcode_state->coord_system;
}


uint8_t cm_get_units_mode(GCodeState_t *gcode_state) {
  return gcode_state->units_mode;
}


uint8_t cm_get_select_plane(GCodeState_t *gcode_state) {
  return gcode_state->select_plane;
}


uint8_t cm_get_path_control(GCodeState_t *gcode_state) {
  return gcode_state->path_control;
}


uint8_t cm_get_distance_mode(GCodeState_t *gcode_state) {
  return gcode_state->distance_mode;
}


uint8_t cm_get_feed_rate_mode(GCodeState_t *gcode_state) {
  return gcode_state->feed_rate_mode;
}


uint8_t cm_get_tool(GCodeState_t *gcode_state) {return gcode_state->tool;}


uint8_t cm_get_spindle_mode(GCodeState_t *gcode_state) {
  return gcode_state->spindle_mode;
}


uint8_t cm_get_block_delete_switch() {return cm.gmx.block_delete_switch;}
uint8_t cm_get_runtime_busy() {return mp_get_runtime_busy();}


float cm_get_feed_rate(GCodeState_t *gcode_state) {
  return gcode_state->feed_rate;
}


void cm_set_motion_mode(GCodeState_t *gcode_state, uint8_t motion_mode) {
  gcode_state->motion_mode = motion_mode;
}


void cm_set_spindle_mode(GCodeState_t *gcode_state, uint8_t spindle_mode) {
  gcode_state->spindle_mode = spindle_mode;
}


void cm_set_spindle_speed_parameter(GCodeState_t *gcode_state, float speed) {
  gcode_state->spindle_speed = speed;
}


void cm_set_tool_number(GCodeState_t *gcode_state, uint8_t tool) {
  gcode_state->tool = tool;
}


void cm_set_absolute_override(GCodeState_t *gcode_state,
                              uint8_t absolute_override) {
  gcode_state->absolute_override = absolute_override;
  // must reset offsets if you change absolute override
  cm_set_work_offsets(MODEL);
}


void cm_set_model_linenum(uint32_t linenum) {
  cm.gm.linenum = linenum; // you must first set the model line number,
}


/* Coordinate systems and offsets
 *
 * Functions to get, set and report coordinate systems and work offsets
 * These functions are not part of the NIST defined functions
 */
/*
 * Notes on Coordinate System and Offset functions
 *
 * All positional information in the canonical machine is kept as
 * absolute coords and in canonical units (mm). The offsets are only
 * used to translate in and out of canonical form during
 * interpretation and response.
 *
 * Managing the coordinate systems & offsets is somewhat complicated.
 * The following affect offsets:
 *    - coordinate system selected. 1-9 correspond to G54-G59
 *    - absolute override: forces current move to be interpreted in machine
 *      coordinates: G53 (system 0)
 *    - G92 offsets are added "on top of" the coord system offsets --
 *      if origin_offset_enable
 *    - G28 and G30 moves; these are run in absolute coordinates
 *
 * The offsets themselves are considered static, are kept in cm, and are
 * supposed to be persistent.
 *
 * To reduce complexity and data load the following is done:
 *    - Full data for coordinates/offsets is only accessible by the canonical
 *      machine, not the downstream
 *    - Resolved set of coord and G92 offsets, with per-move exceptions can
 *      be captured as "work_offsets"
 *    - The core gcode context (gm) only knows about the active coord system
 *      and the work offsets
 */

/* Return the currently active coordinate offset for an axis
 *
 * Takes G5x, G92 and absolute override into account to return the
 * active offset for this move
 *
 * This function is typically used to evaluate and set offsets, as
 * opposed to cm_get_work_offset() which merely returns what's in the
 * work_offset[] array.
 */
float cm_get_active_coord_offset(uint8_t axis) {
  if (cm.gm.absolute_override) return 0; // no offset in absolute override mode
  float offset = cm.offset[cm.gm.coord_system][axis];

  if (cm.gmx.origin_offset_enable)
    offset += cm.gmx.origin_offset[axis]; // includes G5x and G92 components

  return offset;
}


/* Return a coord offset from the gcode_state
 *
 * This function accepts as input: MODEL, PLANNER, RUNTIME, ACTIVE_MODEL
 */
float cm_get_work_offset(GCodeState_t *gcode_state, uint8_t axis) {
  return gcode_state->work_offset[axis];
}


/* Capture coord offsets from the model into absolute values in the gcode_state
 *
 * This function accepts as input: MODEL, PLANNER, RUNTIME, ACTIVE_MODEL
 */
void cm_set_work_offsets(GCodeState_t *gcode_state) {
  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    gcode_state->work_offset[axis] = cm_get_active_coord_offset(axis);
}


/* Get position of axis in absolute coordinates
 *
 * This function accepts as input: MODEL, RUNTIME
 *
 * NOTE: Only MODEL and RUNTIME are supported (no PLANNER or bf's)
 * NOTE: Machine position is always returned in mm mode. No units conversion
 * is performed
 */
float cm_get_absolute_position(GCodeState_t *gcode_state, uint8_t axis) {
  if (gcode_state == MODEL) return cm.gmx.position[axis];
  return mp_get_runtime_absolute_position(axis);
}


/* Return work position in external form
 *
 *    ... that means in prevailing units (mm/inch) and with all offsets applied
 *
 * NOTE: This function only works after the gcode_state struct as had the
 * work_offsets setup by calling cm_get_model_coord_offset_vector() first.
 *
 *    This function accepts as input:
 *        MODEL
 *        RUNTIME
 *
 * NOTE: Only MODEL and RUNTIME are supported (no PLANNER or bf's)
 */
float cm_get_work_position(GCodeState_t *gcode_state, uint8_t axis) {
  float position;

  if (gcode_state == MODEL)
    position = cm.gmx.position[axis] - cm_get_active_coord_offset(axis);
  else position = mp_get_runtime_work_position(axis);

  if (gcode_state->units_mode == INCHES) position /= MM_PER_INCH;

  return position;
}


/* Critical helpers
 *
 * Core functions supporting the canonical machining functions
 * These functions are not part of the NIST defined functions
 */

/* Perform final operations for a traverse or feed
 *
 * These routines set the point position in the gcode model.
 *
 * Note: As far as the canonical machine is concerned the final
 * position of a Gcode block (move) is achieved as soon as the move is
 * planned and the move target becomes the new model position.  In
 * reality the planner will (in all likelihood) have only just queued
 * the move for later execution, and the real tool position is still
 * close to the starting point.
 */
void cm_finalize_move() {
  copy_vector(cm.gmx.position, cm.gm.target);        // update model position

  // if in ivnerse time mode reset feed rate so next block requires an
  // explicit feed rate setting
  if (cm.gm.feed_rate_mode == INVERSE_TIME_MODE &&
      cm.gm.motion_mode == MOTION_MODE_STRAIGHT_FEED)
    cm.gm.feed_rate = 0;
}


/// Set endpoint position from final runtime position
void cm_update_model_position_from_runtime() {
  copy_vector(cm.gmx.position, mr.gm.target);
}


/* Set target vector in GM model
 *
 * This is a core routine. It handles:
 *    - conversion of linear units to internal canonical form (mm)
 *    - conversion of relative mode to absolute (internal canonical form)
 *    - translation of work coordinates to machine coordinates (internal
 *      canonical form)
 *    - computation and application of axis modes as so:
 *
 *    DISABLED  - Incoming value is ignored. Target value is not changed
 *    ENABLED   - Convert axis values to canonical format and store as target
 *    INHIBITED - Same processing as ENABLED, but axis will not actually be run
 *    RADIUS    - ABC axis value is provided in Gcode block in linear units
 *              - Target is set to degrees based on axis' Radius value
 *              - Radius mode is only processed for ABC axes. Application to
 *                XYZ is ignored.
 *
 *    Target coordinates are provided in target[]
 *    Axes that need processing are signaled in flag[]
 */

// ESTEE: _calc_ABC is a fix to workaround a gcc compiler bug wherein it runs
// out of spill registers we moved this block into its own function so that we
// get a fresh stack push
// ALDEN: This shows up in avr-gcc 4.7.0 and avr-libc 1.8.0
static float _calc_ABC(uint8_t axis, float target[], float flag[]) {
  if (cm.a[axis].axis_mode == AXIS_STANDARD ||
      cm.a[axis].axis_mode == AXIS_INHIBITED)
    return target[axis];    // no mm conversion - it's in degrees

  return _to_millimeters(target[axis]) * 360 / (2 * M_PI * cm.a[axis].radius);
}


void cm_set_model_target(float target[], float flag[]) {
  uint8_t axis;
  float tmp = 0;

  // process XYZABC for lower modes
  for (axis = AXIS_X; axis <= AXIS_Z; axis++) {
    if (fp_FALSE(flag[axis]) || cm.a[axis].axis_mode == AXIS_DISABLED)
      continue; // skip axis if not flagged for update or its disabled

    else if (cm.a[axis].axis_mode == AXIS_STANDARD ||
             cm.a[axis].axis_mode == AXIS_INHIBITED) {
      if (cm.gm.distance_mode == ABSOLUTE_MODE)
        cm.gm.target[axis] =
          cm_get_active_coord_offset(axis) + _to_millimeters(target[axis]);
      else cm.gm.target[axis] += _to_millimeters(target[axis]);
    }
  }

  // FYI: The ABC loop below relies on the XYZ loop having been run first
  for (axis = AXIS_A; axis <= AXIS_C; axis++) {
    if (fp_FALSE(flag[axis]) || cm.a[axis].axis_mode == AXIS_DISABLED)
      continue; // skip axis if not flagged for update or its disabled
    else tmp = _calc_ABC(axis, target, flag);

    if (cm.gm.distance_mode == ABSOLUTE_MODE)
      // sacidu93's fix to Issue #22
      cm.gm.target[axis] = tmp + cm_get_active_coord_offset(axis);
    else cm.gm.target[axis] += tmp;
  }
}


/* Return error code if soft limit is exceeded
 *
 * Must be called with target properly set in GM struct. Best done
 * after cm_set_model_target().
 *
 * Tests for soft limit for any homed axis if min and max are
 * different values. You can set min and max to 0,0 to disable soft
 * limits for an axis. Also will not test a min or a max if the value
 * is < -1000000 (negative one million). This allows a single end to
 * be tested w/the other disabled, should that requirement ever arise.
 */
stat_t cm_test_soft_limits(float target[]) {
  if (cm.soft_limit_enable)
    for (uint8_t axis = AXIS_X; axis < AXES; axis++) {
      if (!cm.homed[axis]) continue; // don't test axes that are not homed

      if (fp_EQ(cm.a[axis].travel_min, cm.a[axis].travel_max)) continue;

      if (cm.a[axis].travel_min > DISABLE_SOFT_LIMIT &&
          target[axis] < cm.a[axis].travel_min)
        return STAT_SOFT_LIMIT_EXCEEDED;

      if (cm.a[axis].travel_max > DISABLE_SOFT_LIMIT &&
          target[axis] > cm.a[axis].travel_max)
        return STAT_SOFT_LIMIT_EXCEEDED;
    }

  return STAT_OK;
}


/* Canonical machining functions
 *    Values are passed in pre-unit_converted state (from gn structure)
 *    All operations occur on gm (current model state)
 *
 * These are organized by section number (x.x.x) in the order they are
 * found in NIST RS274 NGCv3
 */

// Initialization and Termination (4.3.2)

/// Config init cfg_init() must have been run beforehand
void canonical_machine_init() {
  ACTIVE_MODEL = MODEL; // setup initial Gcode model pointer

  // Init 1/jerk
  for (uint8_t axis = 0; axis < AXES; axis++)
    cm.a[axis].recip_jerk = 1 / (cm.a[axis].jerk_max * JERK_MULTIPLIER);

  // Set gcode defaults
  cm_set_units_mode(cm.units_mode);
  cm_set_coord_system(cm.coord_system);
  cm_select_plane(cm.select_plane);
  cm_set_path_control(cm.path_control);
  cm_set_distance_mode(cm.distance_mode);
  cm_set_feed_rate_mode(UNITS_PER_MINUTE_MODE); // always the default

  // Sub-system inits
  cm_spindle_init();
}


/// Alarm state; send an exception report and stop processing input
stat_t cm_soft_alarm(stat_t status) {
  print_status_message("Soft alarm", status);
  cm.machine_state = MACHINE_ALARM;
  return status;
}


/// Clear soft alarm
stat_t cm_clear() {
  if (cm.cycle_state == CYCLE_OFF)
    cm.machine_state = MACHINE_PROGRAM_STOP;
  else cm.machine_state = MACHINE_CYCLE;

  return STAT_OK;
}


/// Alarm state; send an exception report and shut down machine
stat_t cm_hard_alarm(stat_t status) {
  // Hard stop the motors and the spindle
  stepper_init();
  cm_spindle_control(SPINDLE_OFF);
  print_status_message("HARD ALARM", status);
  cm.machine_state = MACHINE_SHUTDOWN;
  return status;
}

// Representation (4.3.3)
//
// Affect the Gcode model only (asynchronous)
// These functions assume input validation occurred upstream.

/// G17, G18, G19 select axis plane
stat_t cm_select_plane(uint8_t plane) {
  cm.gm.select_plane = plane;
  return STAT_OK;
}


/// G20, G21
stat_t cm_set_units_mode(uint8_t mode) {
  cm.gm.units_mode = mode;        // 0 = inches, 1 = mm.
  return STAT_OK;
}


/// G90, G91
stat_t cm_set_distance_mode(uint8_t mode) {
  cm.gm.distance_mode = mode;     // 0 = absolute mode, 1 = incremental
  return STAT_OK;
}


/* G10 L2 Pn, affects MODEL only, delayed persistence
 *
 * This function applies the offset to the GM model. You can also
 * use $g54x - $g59c config functions to change offsets.
 *
 * It also does not reset the work_offsets which may be
 * accomplished by calling cm_set_work_offsets() immediately
 * afterwards.
 */
stat_t cm_set_coord_offsets(uint8_t coord_system, float offset[],
                            float flag[]) {
  if (coord_system < G54 || coord_system > COORD_SYSTEM_MAX)
    return STAT_INPUT_VALUE_RANGE_ERROR; // you can't set G53

  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    if (fp_TRUE(flag[axis]))
      cm.offset[coord_system][axis] = _to_millimeters(offset[axis]);

  return STAT_OK;
}


// Representation functions that affect gcode model and are queued to planner
// (synchronous)

/// G54-G59
stat_t cm_set_coord_system(uint8_t coord_system) {
  cm.gm.coord_system = coord_system;

  // pass coordinate system in value[0] element
  float value[AXES] = {(float)coord_system, 0, 0, 0, 0, 0};
  // second vector (flags) is not used, so fake it
  mp_queue_command(_exec_offset, value, value);
  return STAT_OK;
}


static void _exec_offset(float *value, float *flag) {
  // coordinate system is passed in value[0] element
  uint8_t coord_system = ((uint8_t)value[0]);
  float offsets[AXES];
  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    offsets[axis] = cm.offset[coord_system][axis] +
      (cm.gmx.origin_offset[axis] * cm.gmx.origin_offset_enable);

  mp_set_runtime_work_offset(offsets);
  cm_set_work_offsets(MODEL); // set work offsets in the Gcode model
}


/* Set the position of a single axis in the model, planner and runtime
 *
 * This command sets an axis/axes to a position provided as an argument.
 * This is useful for setting origins for homing, probing, and other operations.
 *
 *  !!!!! DO NOT CALL THIS FUNCTION WHILE IN A MACHINING CYCLE !!!!!
 *
 * More specifically, do not call this function if there are any moves
 * in the planner or if the runtime is moving. The system must be
 * quiescent or you will introduce positional errors. This is true
 * because the planned / running moves have a different reference
 * frame than the one you are now going to set. These functions should
 * only be called during initialization sequences and during cycles
 * (such as homing cycles) when you know there are no more moves in
 * the planner and that all motion has stopped.  Use
 * cm_get_runtime_busy() to be sure the system is quiescent.
 */
void cm_set_position(uint8_t axis, float position) {
  // TODO: Interlock involving runtime_busy test
  cm.gmx.position[axis] = position;
  cm.gm.target[axis] = position;
  mp_set_planner_position(axis, position);
  mp_set_runtime_position(axis, position);
  mp_set_steps_to_runtime_position();
}


// G28.3 functions and support

/* G28.3 - model, planner and queue to runtime
 *
 * Takes a vector of origins (presumably 0's, but not necessarily) and
 * applies them to all axes where the corresponding position in the
 * flag vector is true (1).
 *
 * This is a 2 step process. The model and planner contexts are set
 * immediately, the runtime command is queued and synchronized with
 * the planner queue. This includes the runtime position and the step
 * recording done by the encoders. At that point any axis that is set
 * is also marked as homed.
 */
stat_t cm_set_absolute_origin(float origin[], float flag[]) {
  float value[AXES];

  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    if (fp_TRUE(flag[axis])) {
      value[axis] = _to_millimeters(origin[axis]);
      cm.gmx.position[axis] = value[axis];         // set model position
      cm.gm.target[axis] = value[axis];            // reset model target
      mp_set_planner_position(axis, value[axis]);  // set mm position
    }

  mp_queue_command(_exec_absolute_origin, value, flag);

  return STAT_OK;
}


static void _exec_absolute_origin(float *value, float *flag) {
  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    if (fp_TRUE(flag[axis])) {
      mp_set_runtime_position(axis, value[axis]);
      cm.homed[axis] = true;    // G28.3 is not considered homed until here
    }

  mp_set_steps_to_runtime_position();
}


/* G92's behave according to NIST 3.5.18 & LinuxCNC G92
 * http://linuxcnc.org/docs/html/gcode/gcode.html#sec:G92-G92.1-G92.2-G92.3
 */

/// G92
stat_t cm_set_origin_offsets(float offset[], float flag[]) {
  // set offsets in the Gcode model extended context
  cm.gmx.origin_offset_enable = 1;
  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    if (fp_TRUE(flag[axis]))
      cm.gmx.origin_offset[axis] = cm.gmx.position[axis] -
        cm.offset[cm.gm.coord_system][axis] - _to_millimeters(offset[axis]);

  // now pass the offset to the callback - setting the coordinate system also
  // applies the offsets
  // pass coordinate system in value[0] element
  float value[AXES] = {cm.gm.coord_system};
  mp_queue_command(_exec_offset, value, value); // second vector is not used

  return STAT_OK;
}


/// G92.1
stat_t cm_reset_origin_offsets() {
  cm.gmx.origin_offset_enable = 0;
  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    cm.gmx.origin_offset[axis] = 0;

  float value[AXES] = {cm.gm.coord_system};
  mp_queue_command(_exec_offset, value, value);

  return STAT_OK;
}


/// G92.2
stat_t cm_suspend_origin_offsets() {
  cm.gmx.origin_offset_enable = 0;
  float value[AXES] = {cm.gm.coord_system};
  mp_queue_command(_exec_offset, value, value);

  return STAT_OK;
}


/// G92.3
stat_t cm_resume_origin_offsets() {
  cm.gmx.origin_offset_enable = 1;
  float value[AXES] = {cm.gm.coord_system};
  mp_queue_command(_exec_offset, value, value);

  return STAT_OK;
}


// Free Space Motion (4.3.4)

/// G0 linear rapid
stat_t cm_straight_traverse(float target[], float flags[]) {
  cm.gm.motion_mode = MOTION_MODE_STRAIGHT_TRAVERSE;
  cm_set_model_target(target, flags);

  // test soft limits
  stat_t status = cm_test_soft_limits(cm.gm.target);
  if (status != STAT_OK) return cm_soft_alarm(status);

  // prep and plan the move
  cm_set_work_offsets(&cm.gm); // capture fully resolved offsets to the state
  cm_cycle_start();            // required for homing & other cycles
  mp_aline(&cm.gm);            // send the move to the planner
  cm_finalize_move();

  return STAT_OK;
}


/// G28.1
stat_t cm_set_g28_position() {
  copy_vector(cm.gmx.g28_position, cm.gmx.position);
  return STAT_OK;
}


/// G28
stat_t cm_goto_g28_position(float target[], float flags[]) {
  cm_set_absolute_override(MODEL, true);
  // move through intermediate point, or skip
  cm_straight_traverse(target, flags);

  // make sure you have an available buffer
  while (!mp_get_planner_buffers_available());

  // execute actual stored move
  float f[] = {1, 1, 1, 1, 1, 1};
  return cm_straight_traverse(cm.gmx.g28_position, f);
}


/// G30.1
stat_t cm_set_g30_position() {
  copy_vector(cm.gmx.g30_position, cm.gmx.position);
  return STAT_OK;
}


/// G30
stat_t cm_goto_g30_position(float target[], float flags[]) {
  cm_set_absolute_override(MODEL, true);
  // move through intermediate point, or skip
  cm_straight_traverse(target, flags);
  // make sure you have an available buffer
  while (!mp_get_planner_buffers_available());
  float f[] = {1, 1, 1, 1, 1, 1};
  // execute actual stored move
  return cm_straight_traverse(cm.gmx.g30_position, f);
}


// Machining Attributes (4.3.5)

/// F parameter (affects MODEL only)
/// Normalize feed rate to mm/min or to minutes if in inverse time mode
stat_t cm_set_feed_rate(float feed_rate) {
  if (cm.gm.feed_rate_mode == INVERSE_TIME_MODE)
    // normalize to minutes (active for this gcode block only)
    cm.gm.feed_rate = 1 / feed_rate;

  else cm.gm.feed_rate = _to_millimeters(feed_rate);

  return STAT_OK;
}


/// G93, G94 (affects MODEL only) See cmFeedRateMode
stat_t cm_set_feed_rate_mode(uint8_t mode) {
  cm.gm.feed_rate_mode = mode;
  return STAT_OK;
}


/// G61, G61.1, G64 (affects MODEL only)
stat_t cm_set_path_control(uint8_t mode) {
  cm.gm.path_control = mode;
  return STAT_OK;
}


// Machining Functions (4.3.6) See arc.c

/// G4, P parameter (seconds)
stat_t cm_dwell(float seconds) {
  cm.gm.parameter = seconds;
  return mp_dwell(seconds);
}


/// G1
stat_t cm_straight_feed(float target[], float flags[]) {
  // trap zero feed rate condition
  if (cm.gm.feed_rate_mode != INVERSE_TIME_MODE && fp_ZERO(cm.gm.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  cm.gm.motion_mode = MOTION_MODE_STRAIGHT_FEED;
  cm_set_model_target(target, flags);

  // test soft limits
  stat_t status = cm_test_soft_limits(cm.gm.target);
  if (status != STAT_OK) return cm_soft_alarm(status);

  // prep and plan the move
  cm_set_work_offsets(&cm.gm); // capture the fully resolved offsets to state
  cm_cycle_start();            // required for homing & other cycles
  status = mp_aline(&cm.gm);   // send the move to the planner
  cm_finalize_move();

  return status;
}


// Spindle Functions (4.3.7) see spindle.c, spindle.h

/* Tool Functions (4.3.8)
 *
 * Note: These functions don't actually do anything for now, and there's a bug
 *       where T and M in different blocks don't work correctly
 */

/// T parameter
stat_t cm_select_tool(uint8_t tool_select) {
  float value[AXES] = {tool_select};
  mp_queue_command(_exec_select_tool, value, value);
  return STAT_OK;
}


static void _exec_select_tool(float *value, float *flag) {
  cm.gm.tool_select = (uint8_t)value[0];
}


/// M6 This might become a complete tool change cycle
stat_t cm_change_tool(uint8_t tool_change) {
  float value[AXES] = {cm.gm.tool_select};
  mp_queue_command(_exec_change_tool, value, value);
  return STAT_OK;
}


static void _exec_change_tool(float *value, float *flag) {
  cm.gm.tool = (uint8_t)value[0];
}


// Miscellaneous Functions (4.3.9)
/// M7
stat_t cm_mist_coolant_control(uint8_t mist_coolant) {
  float value[AXES] = {(float)mist_coolant, 0, 0, 0, 0, 0};
  mp_queue_command(_exec_mist_coolant_control, value, value);
  return STAT_OK;
}


static void _exec_mist_coolant_control(float *value, float *flag) {
  cm.gm.mist_coolant = (uint8_t)value[0];

  if (cm.gm.mist_coolant) gpio_set_bit_on(MIST_COOLANT_BIT);
  else gpio_set_bit_off(MIST_COOLANT_BIT);
}


/// M8, M9
stat_t cm_flood_coolant_control(uint8_t flood_coolant) {
  float value[AXES] = {flood_coolant};
  mp_queue_command(_exec_flood_coolant_control, value, value);
  return STAT_OK;
}


static void _exec_flood_coolant_control(float *value, float *flag) {
  cm.gm.flood_coolant = (uint8_t)value[0];

  if (cm.gm.flood_coolant) gpio_set_bit_on(FLOOD_COOLANT_BIT);
  else {
    gpio_set_bit_off(FLOOD_COOLANT_BIT);
    float vect[] = {}; // turn off mist coolant
    _exec_mist_coolant_control(vect, vect); // M9 special function
  }
}


/* Override enables are kind of a mess in Gcode. This is an attempt to sort
 * them out.  See
 * http://www.linuxcnc.org/docs/2.4/html/gcode_main.html#sec:M50:-Feed-Override
 */

/// M48, M49
stat_t cm_override_enables(uint8_t flag) {
  cm.gmx.feed_rate_override_enable = flag;
  cm.gmx.traverse_override_enable = flag;
  cm.gmx.spindle_override_enable = flag;
  return STAT_OK;
}


/// M50
stat_t cm_feed_rate_override_enable(uint8_t flag) {
  if (fp_TRUE(cm.gf.parameter) && fp_ZERO(cm.gn.parameter))
    cm.gmx.feed_rate_override_enable = false;
  else cm.gmx.feed_rate_override_enable = true;

  return STAT_OK;
}


/// M50.1
stat_t cm_feed_rate_override_factor(uint8_t flag) {
  cm.gmx.feed_rate_override_enable = flag;
  cm.gmx.feed_rate_override_factor = cm.gn.parameter;
  return STAT_OK;
}


/// M50.2
stat_t cm_traverse_override_enable(uint8_t flag) {
  if (fp_TRUE(cm.gf.parameter) && fp_ZERO(cm.gn.parameter))
    cm.gmx.traverse_override_enable = false;
  else cm.gmx.traverse_override_enable = true;

  return STAT_OK;
}


/// M51
stat_t cm_traverse_override_factor(uint8_t flag) {
  cm.gmx.traverse_override_enable = flag;
  cm.gmx.traverse_override_factor = cm.gn.parameter;
  return STAT_OK;
}


/// M51.1
stat_t cm_spindle_override_enable(uint8_t flag) {
  if (fp_TRUE(cm.gf.parameter) && fp_ZERO(cm.gn.parameter))
    cm.gmx.spindle_override_enable = false;
  else cm.gmx.spindle_override_enable = true;

  return STAT_OK;
}


/// M50.1
stat_t cm_spindle_override_factor(uint8_t flag) {
  cm.gmx.spindle_override_enable = flag;
  cm.gmx.spindle_override_factor = cm.gn.parameter;

  return STAT_OK;
}


void cm_message(char *message) {printf(message);}


/* Program Functions (4.3.10)
 *
 * This group implements stop, start, end, and hold.
 * It is extended beyond the NIST spec to handle various situations.
 *
 * cm_program_stop and cm_optional_program_stop are synchronous Gcode
 * commands that are received through the interpreter. They cause all motion
 * to stop at the end of the current command, including spindle motion.
 *
 * Note that the stop occurs at the end of the immediately preceding command
 * (i.e. the stop is queued behind the last command).
 *
 * cm_program_end is a stop that also resets the machine to initial state
 */

/* Feedholds, queue flushes and cycles starts are all related. The request
 * functions set flags for these. The sequencing callback interprets the flags
 * according to the following rules:
 *
 *   A feedhold request received during motion should be honored
 *   A feedhold request received during a feedhold should be ignored and reset
 *   A feedhold request received during a motion stop should be ignored and
 *     reset
 *
 *   A queue flush request received during motion should be ignored but not
 *     reset
 *   A queue flush request received during a feedhold should be deferred until
 *     the feedhold enters a HOLD state (i.e. until deceleration is complete)
 *   A queue flush request received during a motion stop should be honored
 *
 *   A cycle start request received during motion should be ignored and reset
 *   A cycle start request received during a feedhold should be deferred until
 *     the feedhold enters a HOLD state (i.e. until deceleration is complete)
 *     If a queue flush request is also present the queue flush should be done
 *     first
 *   A cycle start request received during a motion stop should be honored and
 *     should start to run anything in the planner queue
 */


/// Initiate a feedhold right now
void cm_request_feedhold() {cm.feedhold_requested = true;}
void cm_request_queue_flush() {cm.queue_flush_requested = true;}
void cm_request_cycle_start() {cm.cycle_start_requested = true;}


/// Process feedholds, cycle starts & queue flushes
stat_t cm_feedhold_sequencing_callback() {
  if (cm.feedhold_requested) {
    if (cm.motion_state == MOTION_RUN && cm.hold_state == FEEDHOLD_OFF) {
      cm_set_motion_state(MOTION_HOLD);
      cm.hold_state = FEEDHOLD_SYNC;     // invokes hold from aline execution
    }

    cm.feedhold_requested = false;
  }

  if (cm.queue_flush_requested) {
    if ((cm.motion_state == MOTION_STOP ||
         (cm.motion_state == MOTION_HOLD && cm.hold_state == FEEDHOLD_HOLD)) &&
        !cm_get_runtime_busy()) {
      cm.queue_flush_requested = false;
      cm_queue_flush();
    }
  }

  bool feedhold_processing =
    cm.hold_state == FEEDHOLD_SYNC ||
    cm.hold_state == FEEDHOLD_PLAN ||
    cm.hold_state == FEEDHOLD_DECEL;

  if (cm.cycle_start_requested && !cm.queue_flush_requested &&
      !feedhold_processing) {
    cm.cycle_start_requested = false;
    cm.hold_state = FEEDHOLD_END_HOLD;
    cm_cycle_start();
    mp_end_hold();
  }

  return STAT_OK;
}


stat_t cm_queue_flush() {
  if (cm_get_runtime_busy()) return STAT_COMMAND_NOT_ACCEPTED;

  usart_rx_flush();                          // flush serial queues
  mp_flush_planner();                        // flush planner queue

  // Note: The following uses low-level mp calls for absolute position.
  //   It could also use cm_get_absolute_position(RUNTIME, axis);
  for (uint8_t axis = AXIS_X; axis < AXES; axis++)
    // set mm from mr
    cm_set_position(axis, mp_get_runtime_absolute_position(axis));

  float value[AXES] = {MACHINE_PROGRAM_STOP};
  _exec_program_finalize(value, value);    // finalize now, not later

  return STAT_OK;
}


/* Program and cycle state functions
 *
 * cm_program_end() implements M2 and M30
 * The END behaviors are defined by NIST 3.6.1 are:
 *    1. Axis offsets are set to zero (like G92.2) and origin offsets are set
 *       to the default (like G54)
 *    2. Selected plane is set to CANON_PLANE_XY (like G17)
 *    3. Distance mode is set to MODE_ABSOLUTE (like G90)
 *    4. Feed rate mode is set to UNITS_PER_MINUTE (like G94)
 *    5. Feed and speed overrides are set to ON (like M48)
 *    6. Cutter compensation is turned off (like G40)
 *    7. The spindle is stopped (like M5)
 *    8. The current motion mode is set to G_1 (like G1)
 *    9. Coolant is turned off (like M9)
 *
 * cm_program_end() implments things slightly differently:
 *    1. Axis offsets are set to G92.1 CANCEL offsets
 *       (instead of using G92.2 SUSPEND Offsets)
 *       Set default coordinate system (uses $gco, not G54)
 *    2. Selected plane is set to default plane ($gpl)
 *       (instead of setting it to G54)
 *    3. Distance mode is set to MODE_ABSOLUTE (like G90)
 *    4. Feed rate mode is set to UNITS_PER_MINUTE (like G94)
 *    5. Not implemented
 *    6. Not implemented
 *    7. The spindle is stopped (like M5)
 *    8. Motion mode is canceled like G80 (not set to G1)
 *    9. Coolant is turned off (like M9)
 *    +  Default INCHES or MM units mode is restored ($gun)
 */
static void _exec_program_finalize(float *value, float *flag) {
  cm.machine_state = (uint8_t)value[0];
  cm_set_motion_state(MOTION_STOP);
  if (cm.cycle_state == CYCLE_MACHINING)
    cm.cycle_state = CYCLE_OFF;     // don't end cycle if homing, probing, etc.

  cm.hold_state = FEEDHOLD_OFF;     // end feedhold (if in feed hold)
  cm.cycle_start_requested = false; // cancel any pending cycle start request
  mp_zero_segment_velocity();       // for reporting purposes

  // perform the following resets if it's a program END
  if (cm.machine_state == MACHINE_PROGRAM_END) {
    cm_reset_origin_offsets();           // G92.1 - we do G91.1 instead of G92.2
    cm_set_coord_system(cm.coord_system);  // reset to default coordinate system
    cm_select_plane(cm.select_plane);      // reset to default arc plane
    cm_set_distance_mode(cm.distance_mode);
    cm_spindle_control(SPINDLE_OFF);                 // M5
    cm_flood_coolant_control(false);                 // M9
    cm_set_feed_rate_mode(UNITS_PER_MINUTE_MODE);    // G94
    cm_set_motion_mode(MODEL, MOTION_MODE_CANCEL_MOTION_MODE);
  }
}


/// Do a cycle start right now
void cm_cycle_start() {
  cm.machine_state = MACHINE_CYCLE;

  // don't (re)start homing, probe or other canned cycles
  if (cm.cycle_state == CYCLE_OFF) cm.cycle_state = CYCLE_MACHINING;
}


/// Do a cycle end right now
void cm_cycle_end() {
  if (cm.cycle_state != CYCLE_OFF) {
    float value[AXES] = {MACHINE_PROGRAM_STOP};
    _exec_program_finalize(value, value);
  }
}



/// M0  Queue a program stop
void cm_program_stop() {
  float value[AXES] = {MACHINE_PROGRAM_STOP};
  mp_queue_command(_exec_program_finalize, value, value);
}


/// M1
void cm_optional_program_stop() {
  float value[AXES] = {MACHINE_PROGRAM_STOP};
  mp_queue_command(_exec_program_finalize, value, value);
}


/// M2, M30
void cm_program_end() {
  float value[AXES] = {MACHINE_PROGRAM_END};
  mp_queue_command(_exec_program_finalize, value, value);
}


/// return ASCII char for axis given the axis number
char cm_get_axis_char(const int8_t axis) {
  char axis_char[] = "XYZABC";
  if (axis < 0 || axis > AXES) return ' ';
  return axis_char[axis];
}


/* Jerk functions
 *
 * Jerk values can be rather large, often in the billions. This makes
 * for some pretty big numbers for people to deal with. Jerk values
 * are stored in the system in truncated format; values are divided by
 * 1,000,000 then reconstituted before use.
 *
 * The axis_jerk() functions expect the jerk in divided-by 1,000,000 form
 */

/// returns jerk for an axis
float cm_get_axis_jerk(uint8_t axis) {
  return cm.a[axis].jerk_max;
}


/// sets the jerk for an axis, including recirpcal and cached values
void cm_set_axis_jerk(uint8_t axis, float jerk) {
  cm.a[axis].jerk_max = jerk;
  cm.a[axis].recip_jerk = 1 / (jerk * JERK_MULTIPLIER);
}
