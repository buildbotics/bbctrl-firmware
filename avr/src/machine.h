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
#include "status.h"
#include "gcode_state.h"


#define TO_MM(a) (mach_get_units() == INCHES ? (a) * MM_PER_INCH : a)

// Model state getters and setters
uint32_t mach_get_line();
float mach_get_feed_rate();
bool mach_is_inverse_time_mode();
feed_mode_t mach_get_feed_mode();
float mach_get_feed_override();
float mach_get_spindle_override();
motion_mode_t mach_get_motion_mode();
bool mach_is_rapid();
plane_t mach_get_plane();
units_t mach_get_units();
coord_system_t mach_get_coord_system();
bool mach_get_absolute_mode();
path_mode_t mach_get_path_mode();
bool mach_is_exact_stop();
distance_mode_t mach_get_distance_mode();
distance_mode_t mach_get_arc_distance_mode();

void mach_set_motion_mode(motion_mode_t motion_mode);
void mach_set_spindle_mode(spindle_mode_t spindle_mode);
void mach_set_spindle_speed(float speed);
void mach_set_absolute_mode(bool absolute_mode);
void mach_set_line(uint32_t line);

// Coordinate systems and offsets
float mach_get_active_coord_offset(uint8_t axis);
void mach_update_work_offsets();
const float *mach_get_position();
void mach_set_position(const float position[]);
float mach_get_axis_position(uint8_t axis);

// Critical helpers
void mach_calc_target(float target[], const float values[], const bool flags[]);
stat_t mach_test_soft_limits(float target[]);

// machining functions defined by NIST [organized by NIST Gcode doc]

// Initialization and termination (4.3.2)
void machine_init();

// Representation (4.3.3)
void mach_set_plane(plane_t plane);
void mach_set_units(units_t mode);
void mach_set_distance_mode(distance_mode_t mode);
void mach_set_arc_distance_mode(distance_mode_t mode);
void mach_set_coord_offsets(coord_system_t coord_system, float offset[],
                            bool flags[]);

void mach_set_axis_position(unsigned axis, float position);
void mach_set_absolute_origin(float origin[], bool flags[]);

stat_t mach_zero_all();
stat_t mach_zero_axis(unsigned axis);

void mach_set_coord_system(coord_system_t coord_system);
void mach_set_origin_offsets(float offset[], bool flags[]);
void mach_reset_origin_offsets();
void mach_suspend_origin_offsets();
void mach_resume_origin_offsets();

// Free Space Motion (4.3.4)
stat_t mach_plan_line(float target[]);
stat_t mach_rapid(float target[], bool flags[]);
void mach_set_g28_position();
stat_t mach_goto_g28_position(float target[], bool flags[]);
void mach_set_g30_position();
stat_t mach_goto_g30_position(float target[], bool flags[]);

// Machining Attributes (4.3.5)
void mach_set_feed_rate(float feed_rate);
void mach_set_feed_mode(feed_mode_t mode);
void mach_set_path_mode(path_mode_t mode);

// Machining Functions (4.3.6) see arc.h
stat_t mach_feed(float target[], bool flags[]);
stat_t mach_dwell(float seconds);

// Spindle Functions (4.3.7) see spindle.h

// Tool Functions (4.3.8)
void mach_select_tool(uint8_t tool);
void mach_change_tool(bool x);

// Miscellaneous Functions (4.3.9)
void mach_mist_coolant_control(bool mist_coolant);
void mach_flood_coolant_control(bool flood_coolant);

void mach_set_feed_override(float override);
void mach_set_spindle_override(float override);
void mach_override_enables(bool flag);
void mach_feed_override_enable(bool flag);
void mach_spindle_override_enable(bool flag);

void mach_message(const char *message);

// Program Functions (4.3.10)
void mach_program_stop();
void mach_optional_program_stop();
void mach_pallet_change_stop();
void mach_program_end();
