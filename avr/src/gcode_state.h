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

#pragma once

#include "config.h"
#include "pgmspace.h"

#include <stdint.h>
#include <stdbool.h>


/* The difference between next_action_t and motion_mode_t is that
 * next_action_t is used by the current block, and may carry non-modal
 * commands, whereas motion_mode_t persists across blocks as G modal group 1
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
} next_action_t;


typedef enum {                        // G Modal Group 1
  MOTION_MODE_RAPID,                  // G0 - rapid
  MOTION_MODE_FEED,                   // G1 - straight feed
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
} motion_mode_t;


typedef enum { // plane - translates to:
  //                    axis_0    axis_1    axis_2
  PLANE_XY,     // G17    X         Y         Z
  PLANE_XZ,     // G18    X         Z         Y
  PLANE_YZ,     // G19    Y         Z         X
} plane_t;


typedef enum {
  INCHES,        // G20
  MILLIMETERS,   // G21
  DEGREES,       // ABC axes (this value used for displays only)
} units_t;


typedef enum {
  ABSOLUTE_COORDS,                // machine coordinate system
  G54, G55, G56, G57, G58, G59,
} coord_system_t;


/// G Modal Group 13
typedef enum {
  PATH_EXACT_PATH,                // G61 hits corners but stops only if needed
  PATH_EXACT_STOP,                // G61.1 stops at all corners
  PATH_CONTINUOUS,                // G64 and typically the default mode
} path_mode_t;


typedef enum {
  ABSOLUTE_MODE,                  // G90
  INCREMENTAL_MODE,               // G91
} distance_mode_t;


typedef enum {
  UNITS_PER_MINUTE_MODE,          // G94
  INVERSE_TIME_MODE,              // G93
  UNITS_PER_REVOLUTION_MODE,      // G95 (unimplemented)
} feed_mode_t;


typedef enum {
  ORIGIN_OFFSET_SET,      // G92 - set origin offsets
  ORIGIN_OFFSET_CANCEL,   // G92.1 - zero out origin offsets
  ORIGIN_OFFSET_SUSPEND,  // G92.2 - do not apply offsets, but preserve values
  ORIGIN_OFFSET_RESUME,   // G92.3 - resume application of the suspended offsets
} origin_offset_t;


typedef enum {
  PROGRAM_STOP,
  PROGRAM_OPTIONAL_STOP,
  PROGRAM_PALLET_CHANGE_STOP,
  PROGRAM_END,
} program_flow_t;


/// spindle state settings
typedef enum {
  SPINDLE_OFF,
  SPINDLE_CW,
  SPINDLE_CCW,
} spindle_mode_t;


/// mist and flood coolant states
typedef enum {
  COOLANT_OFF,        // all coolant off
  COOLANT_ON,         // request coolant on or indicate both coolants are on
  COOLANT_MIST,       // indicates mist coolant on
  COOLANT_FLOOD,      // indicates flood coolant on
} coolant_state_t;


/// used for spindle and arc dir
typedef enum {
  DIRECTION_CW,
  DIRECTION_CCW,
} direction_t;


/* Gcode model
 *
 * - mach.gm is the core Gcode model state. It keeps the internal gcode
 *     state model in normalized, canonical form.  All values are unit
 *     converted (to mm) and in the machine coordinate system
 *     (absolute coordinate system).  Gm is owned by the machine layer and
 *     should be accessed only through mach_*() routines.
 *
 * - parser.gn is used by the gcode parser and is re-initialized for
 *     each gcode block.  It accepts data in the new gcode block in the
 *     formats present in the block (pre-normalized forms). During
 *     initialization some state elements are necessarily restored
 *     from gm.
 *
 * - parser.gf is used by the gcode parser to hold flags for any data that has
 *     changed in gn during the parse.
 */


/// Gcode model state
typedef struct {
#define MEMBER(TYPE, VAR) TYPE VAR;
#include "gcode_state.def"
#undef MEMBER
} gcode_state_t;


typedef struct {
#define MEMBER(TYPE, VAR) bool VAR;
#include "gcode_state.def"
#undef MEMBER
} gcode_flags_t;


PGM_P gs_get_units_pgmstr(units_t mode);
PGM_P gs_get_feed_mode_pgmstr(feed_mode_t mode);
PGM_P gs_get_plane_pgmstr(plane_t plane);
PGM_P gs_get_coord_system_pgmstr(coord_system_t cs);
PGM_P gs_get_path_mode_pgmstr(path_mode_t mode);
PGM_P gs_get_distance_mode_pgmstr(distance_mode_t mode);
