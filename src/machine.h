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
#include "gcode_state.h"

#include <avr/pgmspace.h>


#define TO_MILLIMETERS(a) (mach.gm.units == INCHES ? (a) * MM_PER_INCH : a)


// Note 1: Our G0 omits G4, G30, G53, G92.1, G92.2, G92.3 as these have no axis
// components to error check

typedef struct { // struct to manage mach globals and cycles
  float offset[COORDS + 1][AXES];      // coordinate systems & offsets G53-G59
  float origin_offset[AXES];           // G92 offsets
  bool origin_offset_enable;           // G92 offsets enabled / disabled

  float position[AXES];                // model position
  float g28_position[AXES];            // stored machine position for G28
  float g30_position[AXES];            // stored machine position for G30

  gcode_state_t gm;                    // core gcode model state
} machine_t;


extern machine_t mach;                 // machine controller singleton


// Model state getters and setters
uint32_t mach_get_line();
uint8_t mach_get_tool();
float mach_get_feed_rate();
feed_mode_t mach_get_feed_mode();
float mach_get_feed_override();
float mach_get_spindle_override();
motion_mode_t mach_get_motion_mode();
plane_t mach_get_plane();
units_t mach_get_units();
coord_system_t mach_get_coord_system();
bool mach_get_absolute_mode();
path_mode_t mach_get_path_mode();
distance_mode_t mach_get_distance_mode();
distance_mode_t mach_get_arc_distance_mode();

PGM_P mach_get_units_pgmstr(units_t mode);
PGM_P mach_get_feed_mode_pgmstr(feed_mode_t mode);
PGM_P mach_get_plane_pgmstr(plane_t plane);
PGM_P mach_get_coord_system_pgmstr(coord_system_t cs);
PGM_P mach_get_path_mode_pgmstr(path_mode_t mode);
PGM_P mach_get_distance_mode_pgmstr(distance_mode_t mode);

void mach_set_motion_mode(motion_mode_t motion_mode);
void mach_set_spindle_mode(spindle_mode_t spindle_mode);
void mach_set_spindle_speed(float speed);
void mach_set_tool_number(uint8_t tool);
void mach_set_absolute_mode(bool absolute_mode);
void mach_set_model_line(uint32_t line);

// Coordinate systems and offsets
float mach_get_active_coord_offset(uint8_t axis);
void mach_update_work_offsets();
float mach_get_absolute_position(uint8_t axis);

// Critical helpers
float mach_calc_move_time(const float axis_length[], const float axis_square[]);
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
void mach_set_plane(plane_t plane);
void mach_set_units(units_t mode);
void mach_set_distance_mode(distance_mode_t mode);
void mach_set_coord_offsets(coord_system_t coord_system, float offset[],
                            float flag[]);

void mach_set_axis_position(unsigned axis, float position);
void mach_set_absolute_origin(float origin[], float flag[]);

stat_t mach_zero_all();
stat_t mach_zero_axis(unsigned axis);

void mach_set_coord_system(coord_system_t coord_system);
void mach_set_origin_offsets(float offset[], float flag[]);
void mach_reset_origin_offsets();
void mach_suspend_origin_offsets();
void mach_resume_origin_offsets();

// Free Space Motion (4.3.4)
stat_t mach_rapid(float target[], float flags[]);
void mach_set_g28_position();
stat_t mach_goto_g28_position(float target[], float flags[]);
void mach_set_g30_position();
stat_t mach_goto_g30_position(float target[], float flags[]);

// Machining Attributes (4.3.5)
void mach_set_feed_rate(float feed_rate);
void mach_set_feed_mode(feed_mode_t mode);
void mach_set_path_mode(path_mode_t mode);

// Machining Functions (4.3.6)
stat_t mach_feed(float target[], float flags[]);
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
void mach_feed_override_enable(bool flag);
void mach_feed_override_factor(bool flag);
void mach_spindle_override_enable(bool flag);
void mach_spindle_override_factor(bool flag);

void mach_message(const char *message);

// Program Functions (4.3.10)
void mach_program_stop();
void mach_optional_program_stop();
void mach_pallet_change_stop();
void mach_program_end();

char mach_get_axis_char(int8_t axis);
