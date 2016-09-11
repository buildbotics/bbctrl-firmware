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
 * The machine is the layer between the Gcode parser and the motion control code
 * for a specific robot. It keeps state and executes commands - passing the
 * stateless commands to the motion planning layer.
 *
 * Synchronizing command execution:
 *
 * "Synchronous commands" are commands that affect the runtime and need to be
 * synchronized with movement.  Examples include G4 dwells, program stops and
 * ends, and most M commands.  These are queued into the planner queue and
 * execute from the queue.  Synchronous commands work like this:
 *
 *   - Call the mach_xxx_xxx() function which will do any input validation and
 *     return an error if it detects one.
 *
 *   - The mach_ function calls mp_queue_push().  Arguments are a callback to
 *     the _exec_...() function, which is the runtime execution routine, and any
 *     arguments that are needed by the runtime.
 *
 *   - mp_queue_push() stores the callback and the args in a planner buffer.
 *
 *   - When planner execution reaches the buffer it executes the callback w/ the
 *     args.  Take careful note that the callback executes under an interrupt.
 *
 * Note: The synchronous command execution mechanism uses 2 vectors in the bf
 * buffer to store and return values for the callback.  It's obvious, but
 * impractical to pass the entire bf buffer to the callback as some of these
 * commands are actually executed locally and have no buffer.
 */

#include "machine.h"

#include "config.h"
#include "axes.h"
#include "gcode_parser.h"
#include "spindle.h"
#include "coolant.h"
#include "homing.h"

#include "plan/planner.h"
#include "plan/runtime.h"
#include "plan/dwell.h"
#include "plan/arc.h"
#include "plan/line.h"
#include "plan/state.h"


#define DISABLE_SOFT_LIMIT -1000000

typedef struct { // struct to manage mach globals and cycles
  float offset[COORDS + 1][AXES];      // coordinate systems & offsets G53-G59
  float origin_offset[AXES];           // G92 offsets
  bool origin_offset_enable;           // G92 offsets enabled / disabled

  float position[AXES];                // model position
  float g28_position[AXES];            // stored machine position for G28
  float g30_position[AXES];            // stored machine position for G30

  gcode_state_t gm;                    // core gcode model state
} machine_t;


static machine_t mach = {
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

  // State
  .gm = {.motion_mode = MOTION_MODE_CANCEL_MOTION_MODE},
};


// Machine State functions
uint32_t mach_get_line() {return mach.gm.line;}
float mach_get_feed_rate() {return mach.gm.feed_rate;}
feed_mode_t mach_get_feed_mode() {return mach.gm.feed_mode;}


float mach_get_feed_override() {
  return mach.gm.feed_override_enable ? mach.gm.feed_override : 0;
}


float mach_get_spindle_override() {
  return mach.gm.spindle_override_enable ? mach.gm.spindle_override : 0;
}


motion_mode_t mach_get_motion_mode() {return mach.gm.motion_mode;}
plane_t mach_get_plane() {return mach.gm.plane;}
units_t mach_get_units() {return mach.gm.units;}
coord_system_t mach_get_coord_system() {return mach.gm.coord_system;}
bool mach_get_absolute_mode() {return mach.gm.absolute_mode;}
path_mode_t mach_get_path_mode() {return mach.gm.path_mode;}
distance_mode_t mach_get_distance_mode() {return mach.gm.distance_mode;}
distance_mode_t mach_get_arc_distance_mode() {return mach.gm.arc_distance_mode;}


void mach_set_motion_mode(motion_mode_t motion_mode) {
  mach.gm.motion_mode = motion_mode;
}


/// Spindle speed callback from planner queue
static stat_t _exec_spindle_speed(mp_buffer_t *bf) {
  spindle_set_speed(bf->value);
  return STAT_NOOP; // No move queued
}


/// Queue the S parameter to the planner buffer
void mach_set_spindle_speed(float speed) {
  mp_buffer_t *bf = mp_queue_get_tail();
  bf->value = speed;
  mp_queue_push(_exec_spindle_speed, mach_get_line());
}


/// execute the spindle command (called from planner)
static stat_t _exec_spindle_mode(mp_buffer_t *bf) {
  spindle_set_mode(bf->value);
  return STAT_NOOP; // No move queued
}


/// Queue the spindle command to the planner buffer
void mach_set_spindle_mode(spindle_mode_t mode) {
  mp_buffer_t *bf = mp_queue_get_tail();
  bf->value = mode;
  mp_queue_push(_exec_spindle_mode, mach_get_line());
}


void mach_set_absolute_mode(bool absolute_mode) {
  mach.gm.absolute_mode = absolute_mode;
}


void mach_set_line(uint32_t line) {mach.gm.line = line;}


/* Coordinate systems and offsets
 *
 * Functions to get, set and report coordinate systems and work offsets
 * These functions are not part of the NIST defined functions
 *
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
 * This function is typically used to evaluate and set offsets.
 */
float mach_get_active_coord_offset(uint8_t axis) {
  // no offset in absolute override mode
  if (mach.gm.absolute_mode) return 0;
  float offset = mach.offset[mach.gm.coord_system][axis];

  if (mach.origin_offset_enable)
    offset += mach.origin_offset[axis]; // includes G5x and G92 components

  return offset;
}


static stat_t _exec_update_work_offsets(mp_buffer_t *bf) {
  mp_runtime_set_work_offsets(bf->target);
  return STAT_NOOP; // No move queued
}


// Capture coord offsets from the model into absolute values
void mach_update_work_offsets() {
  static float work_offset[AXES] = {0};
  bool same = true;

  for (int axis = 0; axis < AXES; axis++) {
    float offset = mach_get_active_coord_offset(axis);

    if (offset != work_offset[axis]) {
      work_offset[axis] = offset;
      same = false;
    }
  }

  if (!same) {
    mp_buffer_t *bf = mp_queue_get_tail();
    copy_vector(bf->target, work_offset);
    mp_queue_push(_exec_update_work_offsets, mach_get_line());
  }
}


const float *mach_get_position() {return mach.position;}


void mach_set_position(const float position[]) {
  copy_vector(mach.position, position);
}


/*** Get position of axis in absolute coordinates
 *
 * NOTE: Machine position is always returned in mm mode.  No units conversion
 * is performed.
 */
float mach_get_axis_position(uint8_t axis) {return mach.position[axis];}


/* Critical helpers
 *
 * Core functions supporting the machining functions
 * These functions are not part of the NIST defined functions
 */

/* Compute optimal and minimum move times into the gcode_state
 *
 * "Minimum time" is the fastest the move can be performed given the velocity
 * constraints on each participating axis - regardless of the feed rate
 * requested. The minimum time is the time limited by the rate-limiting
 * axis. The minimum time is needed to compute the optimal time and is recorded
 * for possible feed override computation.
 *
 * "Optimal time" is either the time resulting from the requested feed rate or
 * the minimum time if the requested feed rate is not achievable. Optimal times
 * for rapids are always the minimum time.
 *
 * The gcode state must have targets set prior by having mach_set_target(). Axis
 * modes are taken into account by this.
 *
 * The following times are compared and the longest is returned:
 *   - G93 inverse time (if G93 is active)
 *   - time for coordinated move at requested feed rate
 *   - time that the slowest axis would require for the move
 *
 * Sets the following variables in the gcode_state struct - move_time is set to
 * optimal time
 *
 * NIST RS274NGC_v3 Guidance
 *
 * The following is verbatim text from NIST RS274NGC_v3. As I interpret A for
 * moves that combine both linear and rotational movement, the feed rate should
 * apply to the XYZ movement, with the rotational axis (or axes) timed to start
 * and end at the same time the linear move is performed. It is possible under
 * this case for the rotational move to rate-limit the linear move.
 *
 *  2.1.2.5 Feed Rate
 *
 * The rate at which the controlled point or the axes move is nominally a steady
 * rate which may be set by the user. In the Interpreter, the interpretation of
 * the feed rate is as follows unless inverse time feed rate mode is being used
 * in the RS274/NGC view (see Section 3.5.19). The machining functions view of
 * feed rate, as described in Section 4.3.5.1, has conditions under which the
 * set feed rate is applied differently, but none of these is used in the
 * Interpreter.
 *
 * A.  For motion involving one or more of the X, Y, and Z axes (with or without
 *     simultaneous rotational axis motion), the feed rate means length units
 *     per minute along the programmed XYZ path, as if the rotational axes were
 *     not moving.
 *
 * B.  For motion of one rotational axis with X, Y, and Z axes not moving, the
 *     feed rate means degrees per minute rotation of the rotational axis.
 *
 * C.  For motion of two or three rotational axes with X, Y, and Z axes not
 *     moving, the rate is applied as follows. Let dA, dB, and dC be the angles
 *     in degrees through which the A, B, and C axes, respectively, must move.
 *     Let D = sqrt(dA^2 + dB^2 + dC^2). Conceptually, D is a measure of total
 *     angular motion, using the usual Euclidean metric. Let T be the amount of
 *     time required to move through D degrees at the current feed rate in
 *     degrees per minute. The rotational axes should be moved in coordinated
 *     linear motion so that the elapsed time from the start to the end of the
 *     motion is T plus any time required for acceleration or deceleration.
 */
float mach_calc_move_time(const float axis_length[],
                          const float axis_square[]) {
  float max_time = 0;

  // Compute times for feed motion
  if (mach.gm.motion_mode != MOTION_MODE_RAPID) {
    if (mach.gm.feed_mode == INVERSE_TIME_MODE)
      // Feed rate was un-inverted to minutes by mach_set_feed_rate()
      max_time = mach.gm.feed_rate;

    else {
      // Compute length of linear move in millimeters.  Feed rate in mm/min.
      max_time = sqrt(axis_square[AXIS_X] + axis_square[AXIS_Y] +
                      axis_square[AXIS_Z]) / mach.gm.feed_rate;

      // If no linear axes, compute length of multi-axis rotary move in degrees.
      // Feed rate is provided as degrees/min
      if (fp_ZERO(max_time))
        max_time = sqrt(axis_square[AXIS_A] + axis_square[AXIS_B] +
                        axis_square[AXIS_C]) / mach.gm.feed_rate;
    }
  }

  // Compute time required for rate-limiting axis
  for (int axis = 0; axis < AXES; axis++) {
    float time = fabs(axis_length[axis]) /
      (mach.gm.motion_mode == MOTION_MODE_RAPID ? axes[axis].velocity_max :
       axes[axis].feedrate_max);

    if (max_time < time) max_time = time;
  }

  return max_time < MIN_SEGMENT_TIME ? MIN_SEGMENT_TIME : max_time;
}


/*** Calculate target vector
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
void mach_calc_model_target(float target[], const float values[],
                            const bool flags[]) {
  // process XYZABC for lower modes
  for (int axis = AXIS_X; axis <= AXIS_Z; axis++) {
    if (!flags[axis] || axes[axis].axis_mode == AXIS_DISABLED)
      continue; // skip axis if not flagged for update or its disabled

    if (axes[axis].axis_mode == AXIS_STANDARD ||
        axes[axis].axis_mode == AXIS_INHIBITED) {
      if (mach.gm.distance_mode == ABSOLUTE_MODE)
        target[axis] =
          mach_get_active_coord_offset(axis) + TO_MILLIMETERS(values[axis]);
      else target[axis] += TO_MILLIMETERS(values[axis]);
    }
  }

  // Note: The ABC loop below relies on the XYZ loop having been run first
  for (int axis = AXIS_A; axis <= AXIS_C; axis++) {
    if (!flags[axis] || axes[axis].axis_mode == AXIS_DISABLED)
      continue; // skip axis if not flagged for update or its disabled

    float tmp;
    switch (axes[axis].axis_mode) {
    case AXIS_STANDARD:
    case AXIS_INHIBITED:
      tmp = values[axis]; // no mm conversion - it's in degrees

    default:
      tmp = TO_MILLIMETERS(values[axis]) * 360 / (2 * M_PI * axes[axis].radius);
    }

    if (mach.gm.distance_mode == ABSOLUTE_MODE)
      // sacidu93's fix to Issue #22
      target[axis] = tmp + mach_get_active_coord_offset(axis);
    else target[axis] += tmp;
  }
}


/*** Return error code if soft limit is exceeded
 *
 * Must be called with target properly set in GM struct.  Best done
 * after mach_calc_model_target().
 *
 * Tests for soft limit for any homed axis if min and max are
 * different values. You can set min and max to 0,0 to disable soft
 * limits for an axis. Also will not test a min or a max if the value
 * is < -1000000 (negative one million). This allows a single end to
 * be tested w/the other disabled, should that requirement ever arise.
 */
stat_t mach_test_soft_limits(float target[]) {
  for (int axis = 0; axis < AXES; axis++) {
    if (!mach_get_homed(axis)) continue; // don't test axes that arent homed

    if (fp_EQ(axes[axis].travel_min, axes[axis].travel_max)) continue;

    if (axes[axis].travel_min > DISABLE_SOFT_LIMIT &&
        target[axis] < axes[axis].travel_min)
      return STAT_SOFT_LIMIT_EXCEEDED;

    if (axes[axis].travel_max > DISABLE_SOFT_LIMIT &&
        target[axis] > axes[axis].travel_max)
      return STAT_SOFT_LIMIT_EXCEEDED;
  }

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
  for (int axis = 0; axis < AXES; axis++)
    axes_set_jerk(axis, axes[axis].jerk_max);

  // Set gcode defaults
  mach_set_units(GCODE_DEFAULT_UNITS);
  mach_set_coord_system(GCODE_DEFAULT_COORD_SYSTEM);
  mach_set_plane(GCODE_DEFAULT_PLANE);
  mach_set_path_mode(GCODE_DEFAULT_PATH_CONTROL);
  mach_set_distance_mode(GCODE_DEFAULT_DISTANCE_MODE);
  mach_set_arc_distance_mode(GCODE_DEFAULT_ARC_DISTANCE_MODE);
  mach_set_feed_mode(UNITS_PER_MINUTE_MODE); // always the default

  // Sub-system inits
  spindle_init();
  coolant_init();
}


// Representation (4.3.3)
//
// Affect the Gcode model only (asynchronous)
// These functions assume input validation occurred upstream.

/// G17, G18, G19 select axis plane
void mach_set_plane(plane_t plane) {mach.gm.plane = plane;}


/// G20, G21
void mach_set_units(units_t mode) {mach.gm.units = mode;}


/// G90, G91
void mach_set_distance_mode(distance_mode_t mode) {
  mach.gm.distance_mode = mode;
}


/// G90.1, G91.1
void mach_set_arc_distance_mode(distance_mode_t mode) {
  mach.gm.arc_distance_mode = mode;
}


/* G10 L2 Pn, delayed persistence
 *
 * This function applies the offset to the GM model.
 */
void mach_set_coord_offsets(coord_system_t coord_system, float offset[],
                            bool flags[]) {
  if (coord_system < G54 || G59 < coord_system) return;

  for (int axis = 0; axis < AXES; axis++)
    if (flags[axis])
      mach.offset[coord_system][axis] = TO_MILLIMETERS(offset[axis]);
}


/// G54-G59
void mach_set_coord_system(coord_system_t coord_system) {
  mach.gm.coord_system = coord_system;
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
 * the planner and that all motion has stopped.
 */
void mach_set_axis_position(unsigned axis, float position) {
  //if (!mp_is_quiescent()) ALARM(STAT_MACH_NOT_QUIESCENT);
  if (AXES <= axis) return;

  mach.position[axis] = position;
  mp_set_axis_position(axis, position);
  mp_runtime_set_axis_position(axis, position);
  mp_runtime_set_steps_from_position();
}


stat_t mach_zero_all() {
  for (unsigned axis = 0; axis < AXES; axis++) {
    stat_t status = mach_zero_axis(axis);
    if (status != STAT_OK) return status;
  }

  return STAT_OK;
}


stat_t mach_zero_axis(unsigned axis) {
  if (!mp_is_quiescent()) return STAT_MACH_NOT_QUIESCENT;
  if (AXES <= axis) return STAT_INVALID_AXIS;

  mach_set_axis_position(axis, 0);

  return STAT_OK;
}


// G28.3 functions and support
static stat_t _exec_absolute_origin(mp_buffer_t *bf) {
  const float *origin = bf->target;
  const float *flags = bf->unit;

  for (int axis = 0; axis < AXES; axis++)
    if (flags[axis]) {
      mp_runtime_set_axis_position(axis, origin[axis]);
      mach_set_homed(axis, true);  // G28.3 is not considered homed until here
    }

  mp_runtime_set_steps_from_position();

  return STAT_NOOP; // No move queued
}


/* G28.3 - model, planner and queue to runtime
 *
 * Takes a vector of origins (presumably 0's, but not necessarily) and
 * applies them to all axes where the corresponding position in the
 * flags vector is true (1).
 *
 * This is a 2 step process.  The model and planner contexts are set
 * immediately, the runtime command is queued and synchronized with
 * the planner queue.  This includes the runtime position and the step
 * recording done by the encoders.  At that point any axis that is set
 * is also marked as homed.
 */
void mach_set_absolute_origin(float origin[], bool flags[]) {
  float value[AXES];

  for (int axis = 0; axis < AXES; axis++)
    if (flags[axis]) {
      value[axis] = TO_MILLIMETERS(origin[axis]);
      mach.position[axis] = value[axis];           // set model position
      mp_set_axis_position(axis, value[axis]);     // set mm position
    }

  mp_buffer_t *bf = mp_queue_get_tail();
  copy_vector(bf->target, origin);
  copy_vector(bf->unit, flags);
  mp_queue_push(_exec_absolute_origin, mach_get_line());
}


/* G92's behave according to NIST 3.5.18 & LinuxCNC G92
 * http://linuxcnc.org/docs/html/gcode/gcode.html#sec:G92-G92.1-G92.2-G92.3
 */

/// G92
void mach_set_origin_offsets(float offset[], bool flags[]) {
  // set offsets in the Gcode model extended context
  mach.origin_offset_enable = true;
  for (int axis = 0; axis < AXES; axis++)
    if (flags[axis])
      mach.origin_offset[axis] = mach.position[axis] -
        mach.offset[mach.gm.coord_system][axis] - TO_MILLIMETERS(offset[axis]);
}


/// G92.1
void mach_reset_origin_offsets() {
  mach.origin_offset_enable = false;
  for (int axis = 0; axis < AXES; axis++)
    mach.origin_offset[axis] = 0;
}


/// G92.2
void mach_suspend_origin_offsets() {
  mach.origin_offset_enable = false;
}


/// G92.3
void mach_resume_origin_offsets() {
  mach.origin_offset_enable = true;
}


// Free Space Motion (4.3.4)
static stat_t _feed(float values[], bool flags[]) {
  float target[AXES];
  mach_calc_model_target(target, values, flags);

  // test soft limits
  stat_t status = mach_test_soft_limits(target);
  if (status != STAT_OK) return ALARM(status);

  // prep and plan the move
  mach_update_work_offsets();                 // update resolved offsets
  status = mp_aline(target, mach_get_line()); // send move to planner
  copy_vector(mach.position, target);         // update model position

  return status;
}


/// G0 linear rapid
stat_t mach_rapid(float values[], bool flags[]) {
  mach.gm.motion_mode = MOTION_MODE_RAPID;
  return _feed(values, flags);
}


/// G28.1
void mach_set_g28_position() {copy_vector(mach.g28_position, mach.position);}


/// G28
stat_t mach_goto_g28_position(float target[], bool flags[]) {
  mach_set_absolute_mode(true);

  // move through intermediate point, or skip
  mach_rapid(target, flags);

  // execute actual stored move
  bool f[] = {true, true, true, true, true, true};
  return mach_rapid(mach.g28_position, f);
}


/// G30.1
void mach_set_g30_position() {copy_vector(mach.g30_position, mach.position);}


/// G30
stat_t mach_goto_g30_position(float target[], bool flags[]) {
  mach_set_absolute_mode(true);

  // move through intermediate point, or skip
  mach_rapid(target, flags);

  // execute actual stored move
  bool f[] = {true, true, true, true, true, true};
  return mach_rapid(mach.g30_position, f);
}


// Machining Attributes (4.3.5)

/// F parameter
/// Normalize feed rate to mm/min or to minutes if in inverse time mode
void mach_set_feed_rate(float feed_rate) {
  if (mach.gm.feed_mode == INVERSE_TIME_MODE)
    // normalize to minutes (active for this gcode block only)
    mach.gm.feed_rate = feed_rate ? 1 / feed_rate : 0; // Avoid div by zero

  else mach.gm.feed_rate = TO_MILLIMETERS(feed_rate);
}


/// G93, G94
void mach_set_feed_mode(feed_mode_t mode) {
  if (mach.gm.feed_mode == mode) return;
  mach.gm.feed_rate = 0; // Force setting feed rate after changing modes
  mach.gm.feed_mode = mode;
}


/// G61, G61.1, G64
void mach_set_path_mode(path_mode_t mode) {
  mach.gm.path_mode = mode;
}


// Machining Functions (4.3.6) See arc.c

/// G4, P parameter (seconds)
stat_t mach_dwell(float seconds) {
  return mp_dwell(seconds, mach_get_line());
}


/// G1
stat_t mach_feed(float values[], bool flags[]) {
  // trap zero feed rate condition
  if (fp_ZERO(mach.gm.feed_rate) ||
      (mach.gm.feed_mode == INVERSE_TIME_MODE && !parser.gf.feed_rate))
    return STAT_GCODE_FEEDRATE_NOT_SPECIFIED;

  mach.gm.motion_mode = MOTION_MODE_FEED;
  return _feed(values, flags);
}


// Spindle Functions (4.3.7) see spindle.c, spindle.h

// Tool Functions (4.3.8)

/// T parameter
void mach_select_tool(uint8_t tool) {mach.gm.tool = tool;}


static stat_t _exec_change_tool(mp_buffer_t *bf) {
  mp_runtime_set_tool(bf->value);
  return STAT_NOOP; // No move queued
}


/// M6 This might become a complete tool change cycle
void mach_change_tool(bool x) {
  mp_buffer_t *bf = mp_queue_get_tail();
  bf->value = mach.gm.tool;
  mp_queue_push(_exec_change_tool, mach_get_line());
}


// Miscellaneous Functions (4.3.9)
static stat_t _exec_mist_coolant(mp_buffer_t *bf) {
  coolant_set_mist(bf->value);
  return STAT_NOOP; // No move queued
}


/// M7
void mach_mist_coolant_control(bool mist_coolant) {
  mp_buffer_t *bf = mp_queue_get_tail();
  bf->value = mist_coolant;
  mp_queue_push(_exec_mist_coolant, mach_get_line());
}


static stat_t _exec_flood_coolant(mp_buffer_t *bf) {
  coolant_set_flood(bf->value);
  if (!bf->value) coolant_set_mist(false); // M9 special function
  return STAT_NOOP; // No move queued
}


/// M8, M9
void mach_flood_coolant_control(bool flood_coolant) {
  mp_buffer_t *bf = mp_queue_get_tail();
  bf->value = flood_coolant;
  mp_queue_push(_exec_flood_coolant, mach_get_line());
}


/* Override enables are kind of a mess in Gcode. This is an attempt to sort
 * them out.  See
 * http://www.linuxcnc.org/docs/2.4/html/gcode_main.html#sec:M50:-Feed-Override
 */

/// M48, M49
void mach_override_enables(bool flag) {
  mach.gm.feed_override_enable = flag;
  mach.gm.spindle_override_enable = flag;
}


/// M50
void mach_feed_override_enable(bool flag) {
  if (parser.gf.parameter && fp_ZERO(parser.gn.parameter))
    mach.gm.feed_override_enable = false;
  else {
    if (parser.gf.parameter) mach.gm.feed_override = parser.gf.parameter;
    mach.gm.feed_override_enable = true;
  }
}


/// M51
void mach_spindle_override_enable(bool flag) {
  if (parser.gf.parameter && fp_ZERO(parser.gn.parameter))
    mach.gm.spindle_override_enable = false;
  else {
    if (parser.gf.parameter) mach.gm.spindle_override = parser.gf.parameter;
    mach.gm.spindle_override_enable = true;
  }
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
 * commands that are received through the interpreter.  They cause all motion
 * to stop at the end of the current command, including spindle motion.
 *
 * Note that the stop occurs at the end of the immediately preceding command
 * (i.e. the stop is queued behind the last command).
 *
 * mach_program_end is a stop that also resets the machine to initial state
 */


static stat_t _exec_program_stop(mp_buffer_t *bf) {
  // Machine should be stopped at this point.  Go into hold so that a start is
  // needed before executing further instructions.
  mp_state_holding();
  return STAT_NOOP; // No move queued
}


/// M0 Queue a program stop
void mach_program_stop() {
  mp_queue_push(_exec_program_stop, mach_get_line());
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


/*** mach_program_end() implements M2 and M30.  End behaviors are defined by
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
 * mach_program_end() implements things slightly differently:
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


/// M2, M30
void mach_program_end() {
  mach_reset_origin_offsets();      // G92.1 - we do G91.1 instead of G92.2
  mach_set_coord_system(GCODE_DEFAULT_COORD_SYSTEM);
  mach_set_plane(GCODE_DEFAULT_PLANE);
  mach_set_distance_mode(GCODE_DEFAULT_DISTANCE_MODE);
  mach_set_arc_distance_mode(GCODE_DEFAULT_ARC_DISTANCE_MODE);
  mach.gm.spindle_mode = SPINDLE_OFF;
  spindle_set(SPINDLE_OFF, 0);
  mach_flood_coolant_control(false);                 // M9
  mach_set_feed_mode(UNITS_PER_MINUTE_MODE);    // G94
  mach_set_motion_mode(MOTION_MODE_CANCEL_MOTION_MODE);
}
