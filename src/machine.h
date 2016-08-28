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

#pragma once


#include "config.h"
#include "status.h"

#include <stdint.h>
#include <stdbool.h>


#define TO_MILLIMETERS(a) (mach.gm.units_mode == INCHES ? (a) * MM_PER_INCH : a)


/* The difference between machNextAction_t and machMotionMode_ is that
 * machNextAction_t is used by the current block, and may carry non-modal
 * commands, whereas machMotionMode_t persists across blocks as G modal group 1
 */

/// these are in order to optimized CASE statement
typedef enum {
  NEXT_ACTION_DEFAULT,                // Must be zero (invokes motion modes)
  NEXT_ACTION_SEARCH_HOME,            // G28.2 homing cycle
  NEXT_ACTION_SET_ABSOLUTE_ORIGIN,    // G28.3 origin set
  NEXT_ACTION_HOMING_NO_SET,          // G28.4 homing cycle no coord setting
  NEXT_ACTION_SET_G28_POSITION,       // G28.1 set position in abs coordinates
  NEXT_ACTION_GOTO_G28_POSITION,      // G28 go to machine position
  NEXT_ACTION_SET_G30_POSITION,       // G30.1
  NEXT_ACTION_GOTO_G30_POSITION,      // G30
  NEXT_ACTION_SET_COORD_DATA,         // G10
  NEXT_ACTION_SET_ORIGIN_OFFSETS,     // G92
  NEXT_ACTION_RESET_ORIGIN_OFFSETS,   // G92.1
  NEXT_ACTION_SUSPEND_ORIGIN_OFFSETS, // G92.2
  NEXT_ACTION_RESUME_ORIGIN_OFFSETS,  // G92.3
  NEXT_ACTION_DWELL,                  // G4
  NEXT_ACTION_STRAIGHT_PROBE,         // G38.2
} machNextAction_t;


typedef enum {                        // G Modal Group 1
  MOTION_MODE_STRAIGHT_TRAVERSE,      // G0 - straight traverse
  MOTION_MODE_STRAIGHT_FEED,          // G1 - straight feed
  MOTION_MODE_CW_ARC,                 // G2 - clockwise arc feed
  MOTION_MODE_CCW_ARC,                // G3 - counter-clockwise arc feed
  MOTION_MODE_CANCEL_MOTION_MODE,     // G80
  MOTION_MODE_STRAIGHT_PROBE,         // G38.2
  MOTION_MODE_CANNED_CYCLE_81,        // G81 - drilling
  MOTION_MODE_CANNED_CYCLE_82,        // G82 - drilling with dwell
  MOTION_MODE_CANNED_CYCLE_83,        // G83 - peck drilling
  MOTION_MODE_CANNED_CYCLE_84,        // G84 - right hand tapping
  MOTION_MODE_CANNED_CYCLE_85,        // G85 - boring, no dwell, feed out
  MOTION_MODE_CANNED_CYCLE_86,        // G86 - boring, spindle stop, rapid out
  MOTION_MODE_CANNED_CYCLE_87,        // G87 - back boring
  MOTION_MODE_CANNED_CYCLE_88,        // G88 - boring, spindle stop, manual out
  MOTION_MODE_CANNED_CYCLE_89,        // G89 - boring, dwell, feed out
} machMotionMode_t;


typedef enum {   // Used for detecting gcode errors. See NIST section 3.4
  MODAL_GROUP_G0,     // {G10,G28,G28.1,G92}       non-modal axis commands
  MODAL_GROUP_G1,     // {G0,G1,G2,G3,G80}         motion
  MODAL_GROUP_G2,     // {G17,G18,G19}             plane selection
  MODAL_GROUP_G3,     // {G90,G91}                 distance mode
  MODAL_GROUP_G5,     // {G93,G94}                 feed rate mode
  MODAL_GROUP_G6,     // {G20,G21}                 units
  MODAL_GROUP_G7,     // {G40,G41,G42}             cutter radius compensation
  MODAL_GROUP_G8,     // {G43,G49}                 tool length offset
  MODAL_GROUP_G9,     // {G98,G99}                 return mode in canned cycles
  MODAL_GROUP_G12,    // {G54,G55,G56,G57,G58,G59} coordinate system selection
  MODAL_GROUP_G13,    // {G61,G61.1,G64}           path control mode
  MODAL_GROUP_M4,     // {M0,M1,M2,M30,M60}        stopping
  MODAL_GROUP_M6,     // {M6}                      tool change
  MODAL_GROUP_M7,     // {M3,M4,M5}                spindle turning
  MODAL_GROUP_M8,     // {M7,M8,M9}                coolant
  MODAL_GROUP_M9,     // {M48,M49}                 speed/feed override switches
} machModalGroup_t;

#define MODAL_GROUP_COUNT (MODAL_GROUP_M9 + 1)

// Note 1: Our G0 omits G4, G30, G53, G92.1, G92.2, G92.3 as these have no axis
// components to error check

typedef enum { // plane - translates to:
  //                    axis_0    axis_1    axis_2
  PLANE_XY,     // G17    X         Y         Z
  PLANE_XZ,     // G18    X         Z         Y
  PLANE_YZ,     // G19    Y         Z         X
} machPlane_t;


typedef enum {
  INCHES,        // G20
  MILLIMETERS,   // G21
  DEGREES,       // ABC axes (this value used for displays only)
} machUnitsMode_t;


typedef enum {
  ABSOLUTE_COORDS,                // machine coordinate system
  G54, G55, G56, G57, G58, G59,
  MAX_COORDS,
} machCoordSystem_t;

/// G Modal Group 13
typedef enum {
  /// G61 - hits corners but does not stop if it does not need to.
  PATH_EXACT_PATH,
  PATH_EXACT_STOP,                // G61.1 - stops at all corners
  PATH_CONTINUOUS,                // G64 and typically the default mode
} machPathControlMode_t;


typedef enum {
  ABSOLUTE_MODE,                  // G90
  INCREMENTAL_MODE,               // G91
} machDistanceMode_t;


typedef enum {
  INVERSE_TIME_MODE,              // G93
  UNITS_PER_MINUTE_MODE,          // G94
  UNITS_PER_REVOLUTION_MODE,      // G95 (unimplemented)
} machFeedRateMode_t;


typedef enum {
  ORIGIN_OFFSET_SET,      // G92 - set origin offsets
  ORIGIN_OFFSET_CANCEL,   // G92.1 - zero out origin offsets
  ORIGIN_OFFSET_SUSPEND,  // G92.2 - do not apply offsets, but preserve values
  ORIGIN_OFFSET_RESUME,   // G92.3 - resume application of the suspended offsets
} machOriginOffset_t;


typedef enum {
  PROGRAM_STOP,
  PROGRAM_OPTIONAL_STOP,
  PROGRAM_PALLET_CHANGE_STOP,
  PROGRAM_END,
} machProgramFlow_t;


/// spindle state settings
typedef enum {
  SPINDLE_OFF,
  SPINDLE_CW,
  SPINDLE_CCW,
} machSpindleMode_t;


/// mist and flood coolant states
typedef enum {
  COOLANT_OFF,        // all coolant off
  COOLANT_ON,         // request coolant on or indicate both coolants are on
  COOLANT_MIST,       // indicates mist coolant on
  COOLANT_FLOOD,      // indicates flood coolant on
} machCoolantState_t;


/// used for spindle and arc dir
typedef enum {
  DIRECTION_CW,
  DIRECTION_CCW,
} machDirection_t;


/// axis modes (ordered: see _mach_get_feed_time())
typedef enum {
  AXIS_DISABLED,              // kill axis
  AXIS_STANDARD,              // axis in coordinated motion w/standard behaviors
  AXIS_INHIBITED,             // axis is computed but not activated
  AXIS_RADIUS,                // rotary axis calibrated to circumference
  AXIS_MODE_MAX,
} machAxisMode_t; // ordering must be preserved.


/* Gcode model - The following GCodeModel/GCodeInput structs are used:
 *
 * - gm is the core Gcode model state. It keeps the internal gcode
 *     state model in normalized, canonical form. All values are unit
 *     converted (to mm) and in the machine coordinate system
 *     (absolute coordinate system). Gm is owned by the machine layer and
 *     should be accessed only through mach_ routines.
 *
 * - gn is used by the gcode interpreter and is re-initialized for
 *     each gcode block.It accepts data in the new gcode block in the
 *     formats present in the block (pre-normalized forms). During
 *     initialization some state elements are necessarily restored
 *     from gm.
 *
 * - gf is used by the gcode parser interpreter to hold flags for any
 *     data that has changed in gn during the parse. mach.gf.target[]
 *     values are also used by the machine during
 *     set_target().
 */


typedef struct {
  int32_t line;              // gcode block line number
  float target[AXES];        // XYZABC where the move should go
  float work_offset[AXES];   // offset from work coordinate system
  float move_time;           // optimal time for move given axis constraints
} MoveState_t;


/// Gcode model state
typedef struct {
  uint32_t line;                      // Gcode block line number

  uint8_t tool;                       // Tool after T and M6
  uint8_t tool_select;                // T - sets this value

  float feed_rate;                    // F - in mm/min or inverse time mode
  machFeedRateMode_t feed_rate_mode;
  float feed_rate_override_factor;    // 1.0000 x F feed rate.
  bool feed_rate_override_enable;     // M48, M49
  float traverse_override_factor;     // 1.0000 x traverse rate
  bool traverse_override_enable;

  float spindle_speed;                // in RPM
  machSpindleMode_t spindle_mode;
  float spindle_override_factor;      // 1.0000 x S spindle speed
  bool spindle_override_enable;       // true = override enabled

  machMotionMode_t motion_mode;       // Group 1 modal motion
  machPlane_t select_plane;           // G17, G18, G19
  machUnitsMode_t units_mode;         // G20, G21
  machCoordSystem_t coord_system;     // G54-G59 - select coordinate system 1-9
  bool absolute_override;             // G53 true = move in machine coordinates
  machPathControlMode_t path_control;   // G61
  machDistanceMode_t distance_mode;     // G91
  machDistanceMode_t arc_distance_mode; // G91.1

  bool mist_coolant;                  // true = mist on (M7), false = off (M9)
  bool flood_coolant;                 // true = flood on (M8), false = off (M9)

  machNextAction_t next_action;       // handles G group 1 moves & non-modals
  machProgramFlow_t program_flow;     // used only by the gcode_parser

  // unimplemented gcode parameters
  // float cutter_radius;           // D - cutter radius compensation (0 is off)
  // float cutter_length;           // H - cutter length compensation (0 is off)

  // Used for input only
  float target[AXES];                 // XYZABC where the move should go
  bool override_enables;              // feed and spindle enable
  bool tool_change;                   // M6 tool change flag

  float parameter;                    // P - dwell time in sec, G10 coord select

  float arc_radius;                   // R - radius value in arc radius mode
  float arc_offset[3];                // IJK - used by arc commands
} GCodeState_t;


typedef struct {
  machAxisMode_t axis_mode;
  float feedrate_max;    // max velocity in mm/min or deg/min
  float velocity_max;    // max velocity in mm/min or deg/min
  float travel_max;      // max work envelope for soft limits
  float travel_min;      // min work envelope for soft limits
  float jerk_max;        // max jerk (Jm) in mm/min^3 divided by 1 million
  float jerk_homing;     // homing jerk (Jh) in mm/min^3 divided by 1 million
  float recip_jerk;      // reciprocal of current jerk value - with million
  float junction_dev;    // aka cornering delta
  float radius;          // radius in mm for rotary axis modes
  float search_velocity; // homing search velocity
  float latch_velocity;  // homing latch velocity
  float latch_backoff;   // backoff from switches prior to homing latch movement
  float zero_backoff;    // backoff from switches for machine zero
} AxisConfig_t;


typedef struct { // struct to manage mach globals and cycles
  // coordinate systems & offsets absolute (G53) + G54, G55, G56, G57, G58, G59
  float offset[COORDS + 1][AXES];
  float origin_offset[AXES];           // G92 offsets
  bool origin_offset_enable;           // G92 offsets enabled/disabled

  float position[AXES];                // model position (not used in gn or gf)
  float g28_position[AXES];            // stored machine position for G28
  float g30_position[AXES];            // stored machine position for G30

  AxisConfig_t a[AXES];                // settings for axes

  // Model states
  MoveState_t ms;
  GCodeState_t gm;                     // core gcode model state
  GCodeState_t gn;                     // gcode input values
  GCodeState_t gf;                     // gcode input flags
} machine_t;


extern machine_t mach;                 // machine controller singleton


// Model state getters and setters
uint32_t mach_get_line();
machMotionMode_t mach_get_motion_mode();
machCoordSystem_t mach_get_coord_system();
machUnitsMode_t mach_get_units_mode();
machPlane_t mach_get_select_plane();
machPathControlMode_t mach_get_path_control();
machDistanceMode_t mach_get_distance_mode();
machFeedRateMode_t mach_get_feed_rate_mode();
uint8_t mach_get_tool();
machSpindleMode_t mach_get_spindle_mode();
inline float mach_get_spindle_speed() {return mach.gm.spindle_speed;}
float mach_get_feed_rate();

void mach_set_motion_mode(machMotionMode_t motion_mode);
void mach_set_spindle_mode(machSpindleMode_t spindle_mode);
void mach_set_spindle_speed_parameter(float speed);
void mach_set_tool_number(uint8_t tool);
void mach_set_absolute_override(bool absolute_override);
void mach_set_model_line(uint32_t line);

float mach_get_axis_jerk(uint8_t axis);
void mach_set_axis_jerk(uint8_t axis, float jerk);

// Coordinate systems and offsets
float mach_get_active_coord_offset(uint8_t axis);
float mach_get_work_offset(uint8_t axis);
void mach_set_work_offsets();
float mach_get_absolute_position(uint8_t axis);
float mach_get_work_position(uint8_t axis);

// Critical helpers
void mach_calc_move_time(const float axis_length[], const float axis_square[]);
void mach_finalize_move();
stat_t mach_deferred_write_callback();
void mach_set_model_target(float target[], float flag[]);
stat_t mach_test_soft_limits(float target[]);

// machining functions defined by NIST [organized by NIST Gcode doc]

// Initialization and termination (4.3.2)
void machine_init();
/// enter alarm state. returns same status code
stat_t mach_alarm(const char *location, stat_t status);

#define CM_ALARM(CODE) mach_alarm(STATUS_LOCATION, CODE)
#define CM_ASSERT(COND) \
  do {if (!(COND)) CM_ALARM(STAT_INTERNAL_ERROR);} while (0)

// Representation (4.3.3)
void mach_set_plane(machPlane_t plane);
void mach_set_units_mode(machUnitsMode_t mode);
void mach_set_distance_mode(machDistanceMode_t mode);
void mach_set_coord_offsets(machCoordSystem_t coord_system, float offset[],
                          float flag[]);

void mach_set_position(int axis, float position);
void mach_set_absolute_origin(float origin[], float flag[]);

void mach_set_coord_system(machCoordSystem_t coord_system);
void mach_set_origin_offsets(float offset[], float flag[]);
void mach_reset_origin_offsets();
void mach_suspend_origin_offsets();
void mach_resume_origin_offsets();

// Free Space Motion (4.3.4)
stat_t mach_straight_traverse(float target[], float flags[]);
void mach_set_g28_position();
stat_t mach_goto_g28_position(float target[], float flags[]);
void mach_set_g30_position();
stat_t mach_goto_g30_position(float target[], float flags[]);

// Machining Attributes (4.3.5)
void mach_set_feed_rate(float feed_rate);
void mach_set_feed_rate_mode(machFeedRateMode_t mode);
void mach_set_path_control(machPathControlMode_t mode);

// Machining Functions (4.3.6)
stat_t mach_straight_feed(float target[], float flags[]);
stat_t mach_arc_feed(float target[], float flags[], float i, float j, float k,
                     float radius, uint8_t motion_mode);
stat_t mach_dwell(float seconds);

// Spindle Functions (4.3.7) see spindle.h

// Tool Functions (4.3.8)
void mach_select_tool(uint8_t tool);
void mach_change_tool(uint8_t tool);

// Miscellaneous Functions (4.3.9)
void mach_mist_coolant_control(bool mist_coolant);
void mach_flood_coolant_control(bool flood_coolant);

void mach_override_enables(bool flag);
void mach_feed_rate_override_enable(bool flag);
void mach_feed_rate_override_factor(bool flag);
void mach_traverse_override_enable(bool flag);
void mach_traverse_override_factor(bool flag);
void mach_spindle_override_enable(bool flag);
void mach_spindle_override_factor(bool flag);

void mach_message(const char *message);

// Program Functions (4.3.10)
void mach_program_stop();
void mach_optional_program_stop();
void mach_pallet_change_stop();
void mach_program_end();

char mach_get_axis_char(int8_t axis);
