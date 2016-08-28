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
 * machining functions as described in the NIST RS274/NGC v3
 *
 * The machine is the layer between the Gcode parser and
 * the motion control code for a specific robot. It keeps state and
 * executes commands - passing the stateless commands to the motion
 * planning layer.
 *
 * Synchronizing command execution
 *
 *   "Synchronous commands" are commands that affect the runtime need
 *   to be synchronized with movement. Examples include G4 dwells,
 *   program stops and ends, and most M commands.  These are queued
 *   into the planner queue and execute from the queue. Synchronous
 *   commands work like this:
 *
 *     - Call the mach_xxx_xxx() function which will do any input
 *       validation and return an error if it detects one.
 *
 *     - The mach_ function calls mp_queue_command(). Arguments are a
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

#include "machine.h"

#include "config.h"
#include "stepper.h"
#include "spindle.h"
#include "coolant.h"
#include "switch.h"
#include "hardware.h"
#include "util.h"
#include "usart.h"            // for serial queue flush
#include "estop.h"
#include "report.h"
#include "homing.h"

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


#define DISABLE_SOFT_LIMIT -1000000


machine_t mach = {
  // Offsets
  .offset = {
    {}, // ABSOLUTE_COORDS

    {0, 0, 0, 0, 0, 0}, // G54
    {X_TRAVEL_MAX / 2, Y_TRAVEL_MAX / 2, 0, 0, 0, 0}, // G55
    {0, 0, 0, 0, 0, 0}, // G56
    {0, 0, 0, 0, 0, 0}, // G57
    {0, 0, 0, 0, 0, 0}, // G58
    {0, 0, 0, 0, 0, 0}, // G59
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

  // State
  .gm = {.motion_mode = MOTION_MODE_CANCEL_MOTION_MODE},
  .gn = {0},
  .gf = {0},
};


// Command execution callbacks from planner queue
static void _exec_offset(float *value, float *flag);
static void _exec_change_tool(float *value, float *flag);
static void _exec_select_tool(float *value, float *flag);
static void _exec_mist_coolant_control(float *value, float *flag);
static void _exec_flood_coolant_control(float *value, float *flag);
static void _exec_absolute_origin(float *value, float *flag);

// Machine State functions
uint32_t mach_get_line() {return mach.gm.line;}

machMotionMode_t mach_get_motion_mode() {return mach.gm.motion_mode;}
machCoordSystem_t mach_get_coord_system() {return mach.gm.coord_system;}
machUnitsMode_t mach_get_units_mode() {return mach.gm.units_mode;}
machPlane_t mach_get_select_plane() {return mach.gm.select_plane;}
machPathControlMode_t mach_get_path_control() {return mach.gm.path_control;}
machDistanceMode_t mach_get_distance_mode() {return mach.gm.distance_mode;}
machFeedRateMode_t mach_get_feed_rate_mode() {return mach.gm.feed_rate_mode;}
uint8_t mach_get_tool() {return mach.gm.tool;}
machSpindleMode_t mach_get_spindle_mode() {return mach.gm.spindle_mode;}
float mach_get_feed_rate() {return mach.gm.feed_rate;}


void mach_set_motion_mode(machMotionMode_t motion_mode) {
  mach.gm.motion_mode = motion_mode;
}


void mach_set_spindle_mode(machSpindleMode_t spindle_mode) {
  mach.gm.spindle_mode = spindle_mode;
}


void mach_set_spindle_speed_parameter(float speed) {
  mach.gm.spindle_speed = speed;
}


void mach_set_tool_number(uint8_t tool) {mach.gm.tool = tool;}


void mach_set_absolute_override(bool absolute_override) {
  mach.gm.absolute_override = absolute_override;
  // must reset offsets if you change absolute override
  mach_set_work_offsets();
}


void mach_set_model_line(uint32_t line) {mach.gm.line = line;}


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
float mach_get_axis_jerk(uint8_t axis) {
  return mach.a[axis].jerk_max;
}


/// sets the jerk for an axis, including recirpcal and cached values
void mach_set_axis_jerk(uint8_t axis, float jerk) {
  mach.a[axis].jerk_max = jerk;
  mach.a[axis].recip_jerk = 1 / (jerk * JERK_MULTIPLIER);
}


/* Coordinate systems and offsets
 *
 * Functions to get, set and report coordinate systems and work offsets
 * These functions are not part of the NIST defined functions
 */
/*
 * Notes on Coordinate System and Offset functions
 *
 * All positional information in the machine is kept as
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
 * The offsets themselves are considered static, are kept in mach, and are
 * supposed to be persistent.
 *
 * To reduce complexity and data load the following is done:
 *    - Full data for coordinates/offsets is only accessible by the
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
 * opposed to mach_get_work_offset() which merely returns what's in the
 * work_offset[] array.
 */
float mach_get_active_coord_offset(uint8_t axis) {
  // no offset in absolute override mode
  if (mach.gm.absolute_override) return 0;
  float offset = mach.offset[mach.gm.coord_system][axis];

  if (mach.origin_offset_enable)
    offset += mach.origin_offset[axis]; // includes G5x and G92 components

  return offset;
}


/// Return a coord offset
float mach_get_work_offset(uint8_t axis) {
  return mach.ms.work_offset[axis];
}


// Capture coord offsets from the model into absolute values
void mach_set_work_offsets() {
  for (int axis = 0; axis < AXES; axis++)
    mach.ms.work_offset[axis] = mach_get_active_coord_offset(axis);
}


/* Get position of axis in absolute coordinates
 *
 * NOTE: Machine position is always returned in mm mode. No units conversion
 * is performed
 */
float mach_get_absolute_position(uint8_t axis) {
  return mach.position[axis];
}


/* Return work position in prevailing units (mm/inch) and with all offsets
 * applied.
 *
 * NOTE: This function only works after the gcode_state struct has had the
 * work_offsets setup by calling mach_get_model_coord_offset_vector() first.
 */
float mach_get_work_position(uint8_t axis) {
  float position = mach.position[axis] - mach_get_active_coord_offset(axis);

  if (mach.gm.units_mode == INCHES) position /= MM_PER_INCH;

  return position;
}


/* Critical helpers
 *
 * Core functions supporting the machining functions
 * These functions are not part of the NIST defined functions
 */

/* Perform final operations for a traverse or feed
 *
 * These routines set the point position in the gcode model.
 *
 * Note: As far as the machine is concerned the final
 * position of a Gcode block (move) is achieved as soon as the move is
 * planned and the move target becomes the new model position.  In
 * reality the planner will (in all likelihood) have only just queued
 * the move for later execution, and the real tool position is still
 * close to the starting point.
 */
void mach_finalize_move() {
  copy_vector(mach.position, mach.ms.target);        // update model position

  // if in inverse time mode reset feed rate so next block requires an
  // explicit feed rate setting
  if (mach.gm.feed_rate_mode == INVERSE_TIME_MODE &&
      mach.gm.motion_mode == MOTION_MODE_STRAIGHT_FEED)
    mach.gm.feed_rate = 0;
}


/* Compute optimal and minimum move times into the gcode_state
 *
 * "Minimum time" is the fastest the move can be performed given
 * the velocity constraints on each participating axis - regardless
 * of the feed rate requested. The minimum time is the time limited
 * by the rate-limiting axis. The minimum time is needed to compute
 * the optimal time and is recorded for possible feed override
 * computation.
 *
 * "Optimal time" is either the time resulting from the requested
 * feed rate or the minimum time if the requested feed rate is not
 * achievable. Optimal times for traverses are always the minimum
 * time.
 *
 * The gcode state must have targets set prior by having
 * mach_set_target(). Axis modes are taken into account by this.
 *
 * The following times are compared and the longest is returned:
 *   - G93 inverse time (if G93 is active)
 *   - time for coordinated move at requested feed rate
 *   - time that the slowest axis would require for the move
 *
 * Sets the following variables in the gcode_state struct
 *   - move_time is set to optimal time
 *
 * NIST RS274NGC_v3 Guidance
 *
 * The following is verbatim text from NIST RS274NGC_v3. As I
 * interpret A for moves that combine both linear and rotational
 * movement, the feed rate should apply to the XYZ movement, with
 * the rotational axis (or axes) timed to start and end at the same
 * time the linear move is performed. It is possible under this
 * case for the rotational move to rate-limit the linear move.
 *
 *  2.1.2.5 Feed Rate
 *
 * The rate at which the controlled point or the axes move is
 * nominally a steady rate which may be set by the user. In the
 * Interpreter, the interpretation of the feed rate is as follows
 * unless inverse time feed rate mode is being used in the
 * RS274/NGC view (see Section 3.5.19). The machining
 * functions view of feed rate, as described in Section 4.3.5.1,
 * has conditions under which the set feed rate is applied
 * differently, but none of these is used in the Interpreter.
 *
 * A.  For motion involving one or more of the X, Y, and Z axes
 *     (with or without simultaneous rotational axis motion), the
 *     feed rate means length units per minute along the programmed
 *     XYZ path, as if the rotational axes were not moving.
 *
 * B.  For motion of one rotational axis with X, Y, and Z axes not
 *     moving, the feed rate means degrees per minute rotation of
 *     the rotational axis.
 *
 * C.  For motion of two or three rotational axes with X, Y, and Z
 *     axes not moving, the rate is applied as follows. Let dA, dB,
 *     and dC be the angles in degrees through which the A, B, and
 *     C axes, respectively, must move.  Let D = sqrt(dA^2 + dB^2 +
 *     dC^2). Conceptually, D is a measure of total angular motion,
 *     using the usual Euclidean metric. Let T be the amount of
 *     time required to move through D degrees at the current feed
 *     rate in degrees per minute. The rotational axes should be
 *     moved in coordinated linear motion so that the elapsed time
 *     from the start to the end of the motion is T plus any time
 *     required for acceleration or deceleration.
 */
void mach_calc_move_time(const float axis_length[], const float axis_square[]) {
  float inv_time = 0;           // inverse time if doing a feed in G93 mode
  float xyz_time = 0;           // linear coordinated move at requested feed
  float abc_time = 0;           // rotary coordinated move at requested feed
  float max_time = 0;           // time required for the rate-limiting axis
  float tmp_time = 0;           // used in computation

  // compute times for feed motion
  if (mach.gm.motion_mode != MOTION_MODE_STRAIGHT_TRAVERSE) {
    if (mach.gm.feed_rate_mode == INVERSE_TIME_MODE) {
      // feed rate was un-inverted to minutes by mach_set_feed_rate()
      inv_time = mach.gm.feed_rate;
      mach.gm.feed_rate_mode = UNITS_PER_MINUTE_MODE;

    } else {
      // compute length of linear move in millimeters. Feed rate is provided as
      // mm/min
      xyz_time = sqrt(axis_square[AXIS_X] + axis_square[AXIS_Y] +
                      axis_square[AXIS_Z]) / mach.gm.feed_rate;

      // if no linear axes, compute length of multi-axis rotary move in degrees.
      // Feed rate is provided as degrees/min
      if (fp_ZERO(xyz_time))
        abc_time = sqrt(axis_square[AXIS_A] + axis_square[AXIS_B] +
                        axis_square[AXIS_C]) / mach.gm.feed_rate;
    }
  }

  for (uint8_t axis = 0; axis < AXES; axis++) {
    if (mach.gm.motion_mode == MOTION_MODE_STRAIGHT_TRAVERSE)
      tmp_time = fabs(axis_length[axis]) / mach.a[axis].velocity_max;

    else // MOTION_MODE_STRAIGHT_FEED
      tmp_time = fabs(axis_length[axis]) / mach.a[axis].feedrate_max;

    max_time = max(max_time, tmp_time);
  }

  mach.ms.move_time = max4(inv_time, max_time, xyz_time, abc_time);
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
  if (mach.a[axis].axis_mode == AXIS_STANDARD ||
      mach.a[axis].axis_mode == AXIS_INHIBITED)
    return target[axis];    // no mm conversion - it's in degrees

  return TO_MILLIMETERS(target[axis]) * 360 / (2 * M_PI * mach.a[axis].radius);
}


void mach_set_model_target(float target[], float flag[]) {
  float tmp = 0;

  // process XYZABC for lower modes
  for (int axis = AXIS_X; axis <= AXIS_Z; axis++) {
    if (fp_FALSE(flag[axis]) || mach.a[axis].axis_mode == AXIS_DISABLED)
      continue; // skip axis if not flagged for update or its disabled

    else if (mach.a[axis].axis_mode == AXIS_STANDARD ||
             mach.a[axis].axis_mode == AXIS_INHIBITED) {
      if (mach.gm.distance_mode == ABSOLUTE_MODE)
        mach.ms.target[axis] =
          mach_get_active_coord_offset(axis) + TO_MILLIMETERS(target[axis]);
      else mach.ms.target[axis] += TO_MILLIMETERS(target[axis]);
    }
  }

  // NOTE: The ABC loop below relies on the XYZ loop having been run first
  for (int axis = AXIS_A; axis <= AXIS_C; axis++) {
    if (fp_FALSE(flag[axis]) || mach.a[axis].axis_mode == AXIS_DISABLED)
      continue; // skip axis if not flagged for update or its disabled
    else tmp = _calc_ABC(axis, target, flag);

    if (mach.gm.distance_mode == ABSOLUTE_MODE)
      // sacidu93's fix to Issue #22
      mach.ms.target[axis] = tmp + mach_get_active_coord_offset(axis);
    else mach.ms.target[axis] += tmp;
  }
}


/* Return error code if soft limit is exceeded
 *
 * Must be called with target properly set in GM struct. Best done
 * after mach_set_model_target().
 *
 * Tests for soft limit for any homed axis if min and max are
 * different values. You can set min and max to 0,0 to disable soft
 * limits for an axis. Also will not test a min or a max if the value
 * is < -1000000 (negative one million). This allows a single end to
 * be tested w/the other disabled, should that requirement ever arise.
 */
stat_t mach_test_soft_limits(float target[]) {
#ifdef SOFT_LIMIT_ENABLE
    for (int axis = 0; axis < AXES; axis++) {
      if (!mach_get_homed(axis)) continue; // don't test axes that arent homed

      if (fp_EQ(mach.a[axis].travel_min, mach.a[axis].travel_max)) continue;

      if (mach.a[axis].travel_min > DISABLE_SOFT_LIMIT &&
          target[axis] < mach.a[axis].travel_min)
        return STAT_SOFT_LIMIT_EXCEEDED;

      if (mach.a[axis].travel_max > DISABLE_SOFT_LIMIT &&
          target[axis] > mach.a[axis].travel_max)
        return STAT_SOFT_LIMIT_EXCEEDED;
    }
#endif

  return STAT_OK;
}


/* machining functions
 *    Values are passed in pre-unit_converted state (from gn structure)
 *    All operations occur on gm (current model state)
 *
 * These are organized by section number (x.x.x) in the order they are
 * found in NIST RS274 NGCv3
 */

// Initialization and Termination (4.3.2)

void machine_init() {
  // Init 1/jerk
  for (uint8_t axis = 0; axis < AXES; axis++)
    mach.a[axis].recip_jerk = 1 / (mach.a[axis].jerk_max * JERK_MULTIPLIER);

  // Set gcode defaults
  mach_set_units_mode(GCODE_DEFAULT_UNITS);
  mach_set_coord_system(GCODE_DEFAULT_COORD_SYSTEM);
  mach_set_plane(GCODE_DEFAULT_PLANE);
  mach_set_path_control(GCODE_DEFAULT_PATH_CONTROL);
  mach_set_distance_mode(GCODE_DEFAULT_DISTANCE_MODE);
  mach_set_feed_rate_mode(UNITS_PER_MINUTE_MODE); // always the default

  // Sub-system inits
  mach_spindle_init();
  coolant_init();
}


/// Alarm state; send an exception report and stop processing input
stat_t mach_alarm(const char *location, stat_t code) {
  status_message_P(location, STAT_LEVEL_ERROR, code, 0);
  estop_trigger(ESTOP_ALARM);
  return code;
}


// Representation (4.3.3)
//
// Affect the Gcode model only (asynchronous)
// These functions assume input validation occurred upstream.

/// G17, G18, G19 select axis plane
void mach_set_plane(machPlane_t plane) {mach.gm.select_plane = plane;}


/// G20, G21
void mach_set_units_mode(machUnitsMode_t mode) {mach.gm.units_mode = mode;}


/// G90, G91
void mach_set_distance_mode(machDistanceMode_t mode) {
  mach.gm.distance_mode = mode;
}


/* G10 L2 Pn, delayed persistence
 *
 * This function applies the offset to the GM model.
 *
 * It also does not reset the work_offsets which may be
 * accomplished by calling mach_set_work_offsets() immediately
 * afterwards.
 */
void mach_set_coord_offsets(machCoordSystem_t coord_system, float offset[],
                            float flag[]) {
  if (coord_system < G54 || MAX_COORDS <= coord_system) return;

  for (int axis = 0; axis < AXES; axis++)
    if (fp_TRUE(flag[axis]))
      mach.offset[coord_system][axis] = TO_MILLIMETERS(offset[axis]);
}


// Representation functions that affect gcode model and are queued to planner
// (synchronous)

/// G54-G59
void mach_set_coord_system(machCoordSystem_t coord_system) {
  mach.gm.coord_system = coord_system;

  // pass coordinate system in value[0] element
  float value[AXES] = {coord_system};
  // second vector (flags) is not used, so fake it
  mp_queue_command(_exec_offset, value, value);
}


static void _exec_offset(float *value, float *flag) {
  // coordinate system is passed in value[0] element
  uint8_t coord_system = value[0];
  float offsets[AXES];

  for (int axis = 0; axis < AXES; axis++)
    offsets[axis] = mach.offset[coord_system][axis] +
      mach.origin_offset[axis] * (mach.origin_offset_enable ? 1 : 0);

  mp_set_runtime_work_offset(offsets);
  mach_set_work_offsets(); // set work offsets in the Gcode model
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
 * mp_get_runtime_busy() to be sure the system is quiescent.
 */
void mach_set_position(int axis, float position) {
  // TODO: Interlock involving runtime_busy test
  mach.position[axis] = position;
  mach.ms.target[axis] = position;
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
void mach_set_absolute_origin(float origin[], float flag[]) {
  float value[AXES];

  for (int axis = 0; axis < AXES; axis++)
    if (fp_TRUE(flag[axis])) {
      value[axis] = TO_MILLIMETERS(origin[axis]);
      mach.position[axis] = value[axis];         // set model position
      mach.ms.target[axis] = value[axis];            // reset model target
      mp_set_planner_position(axis, value[axis]);  // set mm position
    }

  mp_queue_command(_exec_absolute_origin, value, flag);
}


static void _exec_absolute_origin(float *value, float *flag) {
  for (int axis = 0; axis < AXES; axis++)
    if (fp_TRUE(flag[axis])) {
      mp_set_runtime_position(axis, value[axis]);
      mach_set_homed(axis, true);  // G28.3 is not considered homed until here
    }

  mp_set_steps_to_runtime_position();
}


/* G92's behave according to NIST 3.5.18 & LinuxCNC G92
 * http://linuxcnc.org/docs/html/gcode/gcode.html#sec:G92-G92.1-G92.2-G92.3
 */

/// G92
void mach_set_origin_offsets(float offset[], float flag[]) {
  // set offsets in the Gcode model extended context
  mach.origin_offset_enable = true;
  for (int axis = 0; axis < AXES; axis++)
    if (fp_TRUE(flag[axis]))
      mach.origin_offset[axis] = mach.position[axis] -
        mach.offset[mach.gm.coord_system][axis] - TO_MILLIMETERS(offset[axis]);

  // now pass the offset to the callback - setting the coordinate system also
  // applies the offsets
  // pass coordinate system in value[0] element
  float value[AXES] = {mach.gm.coord_system};
  mp_queue_command(_exec_offset, value, value); // second vector is not used
}


/// G92.1
void mach_reset_origin_offsets() {
  mach.origin_offset_enable = false;
  for (int axis = 0; axis < AXES; axis++)
    mach.origin_offset[axis] = 0;

  float value[AXES] = {mach.gm.coord_system};
  mp_queue_command(_exec_offset, value, value);
}


/// G92.2
void mach_suspend_origin_offsets() {
  mach.origin_offset_enable = false;
  float value[AXES] = {mach.gm.coord_system};
  mp_queue_command(_exec_offset, value, value);
}


/// G92.3
void mach_resume_origin_offsets() {
  mach.origin_offset_enable = true;
  float value[AXES] = {mach.gm.coord_system};
  mp_queue_command(_exec_offset, value, value);
}


// Free Space Motion (4.3.4)

/// G0 linear rapid
stat_t mach_straight_traverse(float target[], float flags[]) {
  mach.gm.motion_mode = MOTION_MODE_STRAIGHT_TRAVERSE;
  mach_set_model_target(target, flags);

  // test soft limits
  stat_t status = mach_test_soft_limits(mach.ms.target);
  if (status != STAT_OK) return CM_ALARM(status);

  // prep and plan the move
  mach_set_work_offsets(&mach.gm); // capture fully resolved offsets to state
  mach.ms.line = mach.gm.line;     // copy line number
  mp_aline(&mach.ms);              // send the move to the planner
  mach_finalize_move();

  return STAT_OK;
}


/// G28.1
void mach_set_g28_position() {copy_vector(mach.g28_position, mach.position);}


/// G28
stat_t mach_goto_g28_position(float target[], float flags[]) {
  mach_set_absolute_override(true);

  // move through intermediate point, or skip
  mach_straight_traverse(target, flags);

  // make sure you have an available buffer
  mp_wait_for_buffer();

  // execute actual stored move
  float f[] = {1, 1, 1, 1, 1, 1};
  return mach_straight_traverse(mach.g28_position, f);
}


/// G30.1
void mach_set_g30_position() {copy_vector(mach.g30_position, mach.position);}


/// G30
stat_t mach_goto_g30_position(float target[], float flags[]) {
  mach_set_absolute_override(true);

  // move through intermediate point, or skip
  mach_straight_traverse(target, flags);

  // make sure you have an available buffer
  mp_wait_for_buffer();

  // execute actual stored move
  float f[] = {1, 1, 1, 1, 1, 1};
  return mach_straight_traverse(mach.g30_position, f);
}


// Machining Attributes (4.3.5)

/// F parameter
/// Normalize feed rate to mm/min or to minutes if in inverse time mode
void mach_set_feed_rate(float feed_rate) {
  if (mach.gm.feed_rate_mode == INVERSE_TIME_MODE)
    // normalize to minutes (active for this gcode block only)
    mach.gm.feed_rate = 1 / feed_rate;

  else mach.gm.feed_rate = TO_MILLIMETERS(feed_rate);
}


/// G93, G94 See machFeedRateMode
void mach_set_feed_rate_mode(machFeedRateMode_t mode) {
  mach.gm.feed_rate_mode = mode;
}


/// G61, G61.1, G64
void mach_set_path_control(machPathControlMode_t mode) {
  mach.gm.path_control = mode;
}


// Machining Functions (4.3.6) See arc.c

/// G4, P parameter (seconds)
stat_t mach_dwell(float seconds) {
  return mp_dwell(seconds);
}


/// G1
stat_t mach_straight_feed(float target[], float flags[]) {
  // trap zero feed rate condition
  if (mach.gm.feed_rate_mode != INVERSE_TIME_MODE && fp_ZERO(mach.gm.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  mach.gm.motion_mode = MOTION_MODE_STRAIGHT_FEED;
  mach_set_model_target(target, flags);

  // test soft limits
  stat_t status = mach_test_soft_limits(mach.ms.target);
  if (status != STAT_OK) return CM_ALARM(status);

  // prep and plan the move
  mach_set_work_offsets(&mach.gm); // capture fully resolved offsets to state
  mach.ms.line = mach.gm.line;     // copy line number
  status = mp_aline(&mach.ms);     // send the move to the planner
  mach_finalize_move();

  return status;
}


// Spindle Functions (4.3.7) see spindle.c, spindle.h

/* Tool Functions (4.3.8)
 *
 * Note: These functions don't actually do anything for now, and there's a bug
 *       where T and M in different blocks don't work correctly
 */

/// T parameter
void mach_select_tool(uint8_t tool_select) {
  float value[AXES] = {tool_select};
  mp_queue_command(_exec_select_tool, value, value);
}


static void _exec_select_tool(float *value, float *flag) {
  mach.gm.tool_select = value[0];
}


/// M6 This might become a complete tool change cycle
void mach_change_tool(uint8_t tool_change) {
  float value[AXES] = {mach.gm.tool_select};
  mp_queue_command(_exec_change_tool, value, value);
}


static void _exec_change_tool(float *value, float *flag) {
  mach.gm.tool = (uint8_t)value[0];
}


// Miscellaneous Functions (4.3.9)
/// M7
void mach_mist_coolant_control(bool mist_coolant) {
  float value[AXES] = {mist_coolant};
  mp_queue_command(_exec_mist_coolant_control, value, value);
}


static void _exec_mist_coolant_control(float *value, float *flag) {
  coolant_set_mist(mach.gm.mist_coolant = value[0]);
}


/// M8, M9
void mach_flood_coolant_control(bool flood_coolant) {
  float value[AXES] = {flood_coolant};
  mp_queue_command(_exec_flood_coolant_control, value, value);
}


static void _exec_flood_coolant_control(float *value, float *flag) {
  mach.gm.flood_coolant = value[0];

  coolant_set_flood(value[0]);
  if (!value[0]) coolant_set_mist(false); // M9 special function
}


/* Override enables are kind of a mess in Gcode. This is an attempt to sort
 * them out.  See
 * http://www.linuxcnc.org/docs/2.4/html/gcode_main.html#sec:M50:-Feed-Override
 */

/// M48, M49
void mach_override_enables(bool flag) {
  mach.gm.feed_rate_override_enable = flag;
  mach.gm.traverse_override_enable = flag;
  mach.gm.spindle_override_enable = flag;
}


/// M50
void mach_feed_rate_override_enable(bool flag) {
  if (fp_TRUE(mach.gf.parameter) && fp_ZERO(mach.gn.parameter))
    mach.gm.feed_rate_override_enable = false;
  else mach.gm.feed_rate_override_enable = true;
}


/// M50.1
void mach_feed_rate_override_factor(bool flag) {
  mach.gm.feed_rate_override_enable = flag;
  mach.gm.feed_rate_override_factor = mach.gn.parameter;
}


/// M50.2
void mach_traverse_override_enable(bool flag) {
  if (fp_TRUE(mach.gf.parameter) && fp_ZERO(mach.gn.parameter))
    mach.gm.traverse_override_enable = false;
  else mach.gm.traverse_override_enable = true;
}


/// M51
void mach_traverse_override_factor(bool flag) {
  mach.gm.traverse_override_enable = flag;
  mach.gm.traverse_override_factor = mach.gn.parameter;
}


/// M51.1
void mach_spindle_override_enable(bool flag) {
  if (fp_TRUE(mach.gf.parameter) && fp_ZERO(mach.gn.parameter))
    mach.gm.spindle_override_enable = false;
  else mach.gm.spindle_override_enable = true;
}


/// M50.1
void mach_spindle_override_factor(bool flag) {
  mach.gm.spindle_override_enable = flag;
  mach.gm.spindle_override_factor = mach.gn.parameter;
}


void mach_message(const char *message) {
  status_message_P(0, STAT_LEVEL_INFO, STAT_OK, PSTR("%s"), message);
}


/* Program Functions (4.3.10)
 *
 * This group implements stop, start, end, and hold.
 * It is extended beyond the NIST spec to handle various situations.
 *
 * mach_program_stop and mach_optional_program_stop are synchronous Gcode
 * commands that are received through the interpreter. They cause all motion
 * to stop at the end of the current command, including spindle motion.
 *
 * Note that the stop occurs at the end of the immediately preceding command
 * (i.e. the stop is queued behind the last command).
 *
 * mach_program_end is a stop that also resets the machine to initial state
 */


/*** _exec_program_end() implements M2 and M30.  End behaviors are defined by
 * NIST 3.6.1 are:
 *
 *    1. Axis offsets are set to zero (like G92.2) and origin offsets are set
 *       to the default (like G54)
 *    2. Selected plane is set to PLANE_XY (like G17)
 *    3. Distance mode is set to MODE_ABSOLUTE (like G90)
 *    4. Feed rate mode is set to UNITS_PER_MINUTE (like G94)
 *    5. Feed and speed overrides are set to ON (like M48)
 *    6. Cutter compensation is turned off (like G40)
 *    7. The spindle is stopped (like M5)
 *    8. The current motion mode is set to G_1 (like G1)
 *    9. Coolant is turned off (like M9)
 *
 * _exec_program_end() implements things slightly differently:
 *
 *    1. Axis offsets are set to G92.1 CANCEL offsets
 *       (instead of using G92.2 SUSPEND Offsets)
 *       Set default coordinate system
 *    2. Selected plane is set to default plane
 *    3. Distance mode is set to MODE_ABSOLUTE (like G90)
 *    4. Feed rate mode is set to UNITS_PER_MINUTE (like G94)
 *    5. Not implemented
 *    6. Not implemented
 *    7. The spindle is stopped (like M5)
 *    8. Motion mode is canceled like G80 (not set to G1)
 *    9. Coolant is turned off (like M9)
 *    +  Default INCHES or MM units mode is restored
 */
static void _exec_program_end(float *value, float *flag) {
  mach_reset_origin_offsets();      // G92.1 - we do G91.1 instead of G92.2
  mach_set_coord_system(GCODE_DEFAULT_COORD_SYSTEM);
  mach_set_plane(GCODE_DEFAULT_PLANE);
  mach_set_distance_mode(GCODE_DEFAULT_DISTANCE_MODE);
  mach_spindle_control(SPINDLE_OFF);                 // M5
  mach_flood_coolant_control(false);                 // M9
  mach_set_feed_rate_mode(UNITS_PER_MINUTE_MODE);    // G94
  mach_set_motion_mode(MOTION_MODE_CANCEL_MOTION_MODE);
}


/// M0 Queue a program stop
void mach_program_stop() {
  // TODO actually stop program
  // Need to queue a feedhold event in the planner buffer
}


/// M1
void mach_optional_program_stop() {
  // TODO Check for user stop signal
  mach_program_stop();
}


/// M60
void mach_pallet_change_stop() {
  // TODO Emit pallet change signal
  mach_program_stop();
}


/// M2, M30
void mach_program_end() {
  float value[AXES] = {};
  mp_queue_command(_exec_program_end, value, value);
}


/// return ASCII char for axis given the axis number
char mach_get_axis_char(int8_t axis) {
  char axis_char[] = "XYZABC";
  if (axis < 0 || axis > AXES) return ' ';
  return axis_char[axis];
}
