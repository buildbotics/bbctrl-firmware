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


#define TO_MILLIMETERS(a) (cm.gm.units_mode == INCHES ? (a) * MM_PER_INCH : a)

#define DISABLE_SOFT_LIMIT -1000000



/* Machine state model
 *
 * The following main variables track canonical machine state and state
 * transitions.
 *        - cm.machine_state    - overall state of machine and program execution
 *        - cm.cycle_state    - what cycle the machine is executing (or none)
 *        - cm.motion_state    - state of movement
 *
 * Allowed states and combined states:
 *
 *   MACHINE STATE     CYCLE STATE   MOTION_STATE    COMBINED_STATE (FYI)
 *   -------------     ------------  -------------   --------------------
 *   MACHINE_UNINIT    na            na              (U)
 *   MACHINE_READY     CYCLE_OFF     MOTION_STOP     (ROS) RESET-OFF-STOP
 *   MACHINE_PROG_STOP CYCLE_OFF     MOTION_STOP     (SOS) STOP-OFF-STOP
 *   MACHINE_PROG_END  CYCLE_OFF     MOTION_STOP     (EOS) END-OFF-STOP
 *
 *   MACHINE_CYCLE     CYCLE_STARTED MOTION_STOP     (CSS) CYCLE-START-STOP
 *   MACHINE_CYCLE     CYCLE_STARTED MOTION_RUN      (CSR) CYCLE-START-RUN
 *   MACHINE_CYCLE     CYCLE_STARTED MOTION_HOLD     (CSH) CYCLE-START-HOLD
 *   MACHINE_CYCLE     CYCLE_STARTED MOTION_END_HOLD (CSE) CYCLE-START-END_HOLD
 *
 *   MACHINE_CYCLE     CYCLE_HOMING  MOTION_STOP     (CHS) CYCLE-HOMING-STOP
 *   MACHINE_CYCLE     CYCLE_HOMING  MOTION_RUN      (CHR) CYCLE-HOMING-RUN
 *   MACHINE_CYCLE     CYCLE_HOMING  MOTION_HOLD     (CHH) CYCLE-HOMING-HOLD
 *   MACHINE_CYCLE     CYCLE_HOMING  MOTION_END_HOLD (CHE) CYCLE-HOMING-END_HOLD
 */

/// check alignment with messages in config.c / msg_stat strings
typedef enum {
  COMBINED_INITIALIZING, // machine is initializing
  COMBINED_READY,        // machine is ready for use. Also null move STOP state
  COMBINED_ALARM,        // machine in soft alarm state
  COMBINED_PROGRAM_STOP, // program stop or no more blocks
  COMBINED_PROGRAM_END,  // program end
  COMBINED_RUN,          // motion is running
  COMBINED_HOLD,         // motion is holding
  COMBINED_PROBE,        // probe cycle active
  COMBINED_CYCLE,        // machine is running (cycling)
  COMBINED_HOMING,       // homing is treated as a cycle
  COMBINED_SHUTDOWN,     // machine in hard alarm state (shutdown)
} cmCombinedState_t;


typedef enum {
  MACHINE_INITIALIZING, // machine is initializing
  MACHINE_READY,        // machine is ready for use
  MACHINE_ALARM,        // machine in soft alarm state
  MACHINE_PROGRAM_STOP, // program stop or no more blocks
  MACHINE_PROGRAM_END,  // program end
  MACHINE_CYCLE,        // machine is running (cycling)
  MACHINE_SHUTDOWN,     // machine in hard alarm state (shutdown)
} cmMachineState_t;


typedef enum {
  CYCLE_OFF,            // machine is idle
  CYCLE_MACHINING,      // in normal machining cycle
  CYCLE_PROBE,          // in probe cycle
  CYCLE_HOMING,         // homing is treated as a specialized cycle
} cmCycleState_t;


typedef enum {
  MOTION_STOP,          // motion has stopped
  MOTION_RUN,           // machine is in motion
  MOTION_HOLD           // feedhold in progress
} cmMotionState_t;


typedef enum {          // feedhold_state machine
  FEEDHOLD_OFF,         // no feedhold in effect
  FEEDHOLD_SYNC,        // start hold - sync to latest aline segment
  FEEDHOLD_PLAN,        // replan blocks for feedhold
  FEEDHOLD_DECEL,       // decelerate to hold point
  FEEDHOLD_HOLD,        // holding
  FEEDHOLD_END_HOLD     // end hold (transient state to OFF)
} cmFeedholdState_t;


typedef enum {          // applies to cm.homing_state
  HOMING_NOT_HOMED,     // machine is not homed (0=false)
  HOMING_HOMED,         // machine is homed (1=true)
  HOMING_WAITING        // machine waiting to be homed
} cmHomingState_t;


typedef enum {          // applies to cm.probe_state
  PROBE_FAILED,         // probe reached endpoint without triggering
  PROBE_SUCCEEDED,      // probe was triggered, cm.probe_results has position
  PROBE_WAITING,        // probe is waiting to be started
} cmProbeState_t;


/* The difference between NextAction and MotionMode is that NextAction is
 * used by the current block, and may carry non-modal commands, whereas
 * MotionMode persists across blocks (as G modal group 1)
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
  NEXT_ACTION_STRAIGHT_PROBE          // G38.2
} cmNextAction_t;


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
} cmMotionMode_t;


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
} cmModalGroup_t;

#define MODAL_GROUP_COUNT (MODAL_GROUP_M9 + 1)

// Note 1: Our G0 omits G4,G30,G53,G92.1,G92.2,G92.3 as these have no axis
// components to error check

typedef enum { // canonical plane - translates to:
  //                          axis_0    axis_1    axis_2
  CANON_PLANE_XY,     // G17    X          Y          Z
  CANON_PLANE_XZ,     // G18    X          Z          Y
  CANON_PLANE_YZ      // G19    Y          Z          X
} cmCanonicalPlane_t;


typedef enum {
  INCHES,        // G20
  MILLIMETERS,   // G21
  DEGREES        // ABC axes (this value used for displays only)
} cmUnitsMode_t;


typedef enum {
  ABSOLUTE_COORDS,                // machine coordinate system
  G54,                            // G54 coordinate system
  G55,                            // G55 coordinate system
  G56,                            // G56 coordinate system
  G57,                            // G57 coordinate system
  G58,                            // G58 coordinate system
  G59                             // G59 coordinate system
} cmCoordSystem_t;

#define COORD_SYSTEM_MAX G59      // set this manually to the last one

/// G Modal Group 13
typedef enum {
  /// G61 - hits corners but does not stop if it does not need to.
  PATH_EXACT_PATH,
  PATH_EXACT_STOP,                // G61.1 - stops at all corners
  PATH_CONTINUOUS                 // G64 and typically the default mode
} cmPathControlMode_t;


typedef enum {
  ABSOLUTE_MODE,                  // G90
  INCREMENTAL_MODE                // G91
} cmDistanceMode_t;


typedef enum {
  INVERSE_TIME_MODE,              // G93
  UNITS_PER_MINUTE_MODE,          // G94
  UNITS_PER_REVOLUTION_MODE       // G95 (unimplemented)
} cmFeedRateMode_t;


typedef enum {
  ORIGIN_OFFSET_SET,      // G92 - set origin offsets
  ORIGIN_OFFSET_CANCEL,   // G92.1 - zero out origin offsets
  ORIGIN_OFFSET_SUSPEND,  // G92.2 - do not apply offsets, but preserve values
  ORIGIN_OFFSET_RESUME    // G92.3 - resume application of the suspended offsets
} cmOriginOffset_t;


typedef enum {
  PROGRAM_STOP,
  PROGRAM_END
} cmProgramFlow_t;


/// spindle state settings
typedef enum {
  SPINDLE_OFF,
  SPINDLE_CW,
  SPINDLE_CCW
} cmSpindleMode_t;


/// mist and flood coolant states
typedef enum {
  COOLANT_OFF,        // all coolant off
  COOLANT_ON,         // request coolant on or indicate both coolants are on
  COOLANT_MIST,       // indicates mist coolant on
  COOLANT_FLOOD       // indicates flood coolant on
} cmCoolantState_t;


/// used for spindle and arc dir
typedef enum {
  DIRECTION_CW,
  DIRECTION_CCW
} cmDirection_t;


/// axis modes (ordered: see _cm_get_feed_time())
typedef enum {
  AXIS_DISABLED,              // kill axis
  AXIS_STANDARD,              // axis in coordinated motion w/standard behaviors
  AXIS_INHIBITED,             // axis is computed but not activated
  AXIS_RADIUS,                // rotary axis calibrated to circumference
  AXIS_MODE_MAX
} cmAxisMode_t; // ordering must be preserved.


/* Gcode model - The following GCodeModel/GCodeInput structs are used:
 *
 * - gm is the core Gcode model state. It keeps the internal gcode
 *     state model in normalized, canonical form. All values are unit
 *     converted (to mm) and in the machine coordinate system
 *     (absolute coordinate system). Gm is owned by the canonical
 *     machine layer and should be accessed only through cm_ routines.
 *
 * - gn is used by the gcode interpreter and is re-initialized for
 *     each gcode block.It accepts data in the new gcode block in the
 *     formats present in the block (pre-normalized forms). During
 *     initialization some state elements are necessarily restored
 *     from gm.
 *
 * - gf is used by the gcode parser interpreter to hold flags for any
 *     data that has changed in gn during the parse. cm.gf.target[]
 *     values are also used by the canonical machine during
 *     set_target().
 */


typedef struct {
  float target[AXES];        // XYZABC where the move should go
  float work_offset[AXES];   // offset from work coordinate system
  float move_time;           // optimal time for move given axis constraints
} MoveState_t;


/// Gcode model state
typedef struct GCodeState {
  uint32_t linenum;                   // Gcode block line number

  uint8_t tool;                       // Tool after T and M6
  uint8_t tool_select;                // T - sets this value

  float feed_rate;                    // F - in mm/min or inverse time mode
  cmFeedRateMode_t feed_rate_mode;
  float feed_rate_override_factor;    // 1.0000 x F feed rate.
  bool feed_rate_override_enable;     // M48, M49
  float traverse_override_factor;     // 1.0000 x traverse rate
  bool traverse_override_enable;

  float spindle_speed;                // in RPM
  cmSpindleMode_t spindle_mode;
  float spindle_override_factor;      // 1.0000 x S spindle speed
  bool spindle_override_enable;       // true = override enabled

  cmMotionMode_t motion_mode;         // Group 1 modal motion
  cmCanonicalPlane_t select_plane;    // G17, G18, G19
  cmUnitsMode_t units_mode;           // G20, G21
  cmCoordSystem_t coord_system;       // G54-G59 - select coordinate system 1-9
  bool absolute_override;             // G53 true = move in machine coordinates
  cmPathControlMode_t path_control;   // G61
  cmDistanceMode_t distance_mode;     // G91
  cmDistanceMode_t arc_distance_mode; // G91.1

  bool mist_coolant;                  // true = mist on (M7), false = off (M9)
  bool flood_coolant;                 // true = flood on (M8), false = off (M9)

  cmNextAction_t next_action;         // handles G group 1 moves & non-modals
  cmProgramFlow_t program_flow;       // used only by the gcode_parser

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


typedef struct cmAxis {
  cmAxisMode_t axis_mode;
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


typedef struct cmSingleton {          // struct to manage cm globals and cycles
  // coordinate systems and offsets absolute (G53) + G54,G55,G56,G57,G58,G59
  float offset[COORDS + 1][AXES];
  float origin_offset[AXES];          // G92 offsets
  bool origin_offset_enable;          // G92 offsets enabled/disabled

  float position[AXES];               // model position (not used in gn or gf)
  float g28_position[AXES];           // stored machine position for G28
  float g30_position[AXES];           // stored machine position for G30

  // settings for axes X,Y,Z,A B,C
  AxisConfig_t a[AXES];

  cmCombinedState_t combined_state;    // combination of states for display
  cmMachineState_t machine_state;
  cmCycleState_t cycle_state;
  cmMotionState_t motion_state;
  cmFeedholdState_t hold_state;        // hold: feedhold sub-state machine
  cmHomingState_t homing_state;        // home: homing cycle sub-state machine
  bool homed[AXES];                    // individual axis homing flags

  cmProbeState_t probe_state;
  float probe_results[AXES];           // probing results

  bool feedhold_requested;             // feedhold character received
  bool queue_flush_requested;          // queue flush character received
  bool cycle_start_requested;          // cycle start character received

  // Model states
  MoveState_t ms;
  GCodeState_t gm;                     // core gcode model state
  GCodeState_t gn;                     // gcode input values
  GCodeState_t gf;                     // gcode input flags
} cmSingleton_t;


extern cmSingleton_t cm;               // canonical machine controller singleton


// Model state getters and setters
uint32_t cm_get_linenum();
cmCombinedState_t cm_get_combined_state();
cmMachineState_t cm_get_machine_state();
cmCycleState_t cm_get_cycle_state();
cmMotionState_t cm_get_motion_state();
cmFeedholdState_t cm_get_hold_state();
cmHomingState_t cm_get_homing_state();
cmMotionMode_t cm_get_motion_mode();
cmCoordSystem_t cm_get_coord_system();
cmUnitsMode_t cm_get_units_mode();
cmCanonicalPlane_t cm_get_select_plane();
cmPathControlMode_t cm_get_path_control();
cmDistanceMode_t cm_get_distance_mode();
cmFeedRateMode_t cm_get_feed_rate_mode();
uint8_t cm_get_tool();
cmSpindleMode_t cm_get_spindle_mode();
bool cm_get_runtime_busy();
float cm_get_feed_rate();

void cm_set_motion_state(cmMotionState_t motion_state);
void cm_set_motion_mode(cmMotionMode_t motion_mode);
void cm_set_spindle_mode(cmSpindleMode_t spindle_mode);
void cm_set_spindle_speed_parameter(float speed);
void cm_set_tool_number(uint8_t tool);
void cm_set_absolute_override(bool absolute_override);
void cm_set_model_linenum(uint32_t linenum);

float cm_get_axis_jerk(uint8_t axis);
void cm_set_axis_jerk(uint8_t axis, float jerk);

// Coordinate systems and offsets
float cm_get_active_coord_offset(uint8_t axis);
float cm_get_work_offset(uint8_t axis);
void cm_set_work_offsets();
float cm_get_absolute_position(uint8_t axis);
float cm_get_work_position(uint8_t axis);

// Critical helpers
void cm_calc_move_time(const float axis_length[], const float axis_square[]);
void cm_update_model_position_from_runtime();
void cm_finalize_move();
stat_t cm_deferred_write_callback();
void cm_set_model_target(float target[], float flag[]);
stat_t cm_test_soft_limits(float target[]);

// Canonical machining functions defined by NIST [organized by NIST Gcode doc]

// Initialization and termination (4.3.2)
void canonical_machine_init();
/// enter hard alarm state. returns same status code
stat_t cm_hard_alarm(stat_t status);
/// enter soft alarm state. returns same status code
stat_t cm_soft_alarm(stat_t status);
stat_t cm_clear();

// Representation (4.3.3)
void cm_set_plane(cmCanonicalPlane_t plane);
void cm_set_units_mode(cmUnitsMode_t mode);
void cm_set_distance_mode(cmDistanceMode_t mode);
void cm_set_coord_offsets(cmCoordSystem_t coord_system, float offset[],
                          float flag[]);

void cm_set_position(int axis, float position);
void cm_set_absolute_origin(float origin[], float flag[]);

void cm_set_coord_system(cmCoordSystem_t coord_system);
void cm_set_origin_offsets(float offset[], float flag[]);
void cm_reset_origin_offsets();
void cm_suspend_origin_offsets();
void cm_resume_origin_offsets();

// Free Space Motion (4.3.4)
stat_t cm_straight_traverse(float target[], float flags[]);
void cm_set_g28_position();
stat_t cm_goto_g28_position(float target[], float flags[]);
void cm_set_g30_position();
stat_t cm_goto_g30_position(float target[], float flags[]);

// Machining Attributes (4.3.5)
void cm_set_feed_rate(float feed_rate);
void cm_set_feed_rate_mode(cmFeedRateMode_t mode);
void cm_set_path_control(cmPathControlMode_t mode);

// Machining Functions (4.3.6)
stat_t cm_straight_feed(float target[], float flags[]);
stat_t cm_arc_feed(float target[], float flags[],
                   float i, float j, float k,
                   float radius, uint8_t motion_mode);
stat_t cm_dwell(float seconds);

// Spindle Functions (4.3.7) see spindle.h

// Tool Functions (4.3.8)
void cm_select_tool(uint8_t tool);
void cm_change_tool(uint8_t tool);

// Miscellaneous Functions (4.3.9)
void cm_mist_coolant_control(bool mist_coolant);
void cm_flood_coolant_control(bool flood_coolant);

void cm_override_enables(bool flag);
void cm_feed_rate_override_enable(bool flag);
void cm_feed_rate_override_factor(bool flag);
void cm_traverse_override_enable(bool flag);
void cm_traverse_override_factor(bool flag);
void cm_spindle_override_enable(bool flag);
void cm_spindle_override_factor(bool flag);

void cm_message(const char *message);

// Program Functions (4.3.10)
void cm_request_feedhold();
void cm_request_queue_flush();
void cm_request_cycle_start();

stat_t cm_feedhold_sequencing_callback();
stat_t cm_queue_flush();

void cm_cycle_start();
void cm_cycle_end();
void cm_feedhold();
void cm_program_stop();
void cm_optional_program_stop();
void cm_program_end();

// Cycles
char cm_get_axis_char(int8_t axis);

// Homing cycles
stat_t cm_homing_cycle_start();
stat_t cm_homing_cycle_start_no_set();
stat_t cm_homing_callback();

// Probe cycles
stat_t cm_straight_probe(float target[], float flags[]);
stat_t cm_probe_callback();
