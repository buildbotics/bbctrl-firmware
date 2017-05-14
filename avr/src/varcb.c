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

#include "usart.h"
#include "machine.h"
#include "spindle.h"
#include "coolant.h"
#include "plan/runtime.h"
#include "plan/state.h"

// Axis
float get_position(int axis) {return mp_runtime_get_axis_position(axis);}


void set_position(int axis, float position) {
  mach_set_axis_position(axis, position);
}


// GCode getters
int32_t get_line() {return mp_runtime_get_line();}
PGM_P get_unit() {return gs_get_units_pgmstr(mach_get_units());}
float get_speed() {return spindle_get_speed();}
float get_feed() {return mach_get_feed_rate();} // TODO get runtime value
uint8_t get_tool() {return mp_runtime_get_tool();}
PGM_P get_feed_mode() {return gs_get_feed_mode_pgmstr(mach_get_feed_mode());}
PGM_P get_plane() {return gs_get_plane_pgmstr(mach_get_plane());}


PGM_P get_coord_system() {
  return gs_get_coord_system_pgmstr(mach_get_coord_system());
}


bool get_abs_override() {return mach_get_absolute_mode();}
PGM_P get_path_mode() {return gs_get_path_mode_pgmstr(mach_get_path_mode());}


PGM_P get_distance_mode() {
  return gs_get_distance_mode_pgmstr(mach_get_distance_mode());
}


PGM_P get_arc_dist_mode() {
  return gs_get_distance_mode_pgmstr(mach_get_arc_distance_mode());
}


float get_feed_override() {return mach_get_feed_override();}
float get_speed_override() {return mach_get_spindle_override();}
bool get_mist_coolant() {return coolant_get_mist();}
bool get_flood_coolant() {return coolant_get_flood();}


// GCode setters
void set_unit(const char *units) {mach_set_units(gs_parse_units(units));}
void set_speed(float speed) {spindle_set_speed(speed);}
void set_feed(float feed) {mach_set_feed_rate(feed);}


void set_tool(uint8_t tool) {
  mp_runtime_set_tool(tool);
  mach_select_tool(tool);
}


void set_feed_mode(const char *mode) {
  mach_set_feed_mode(gs_parse_feed_mode(mode));
}


void set_plane(const char *plane) {mach_set_plane(gs_parse_plane(plane));}


void set_coord_system(const char *cs) {
  mach_set_coord_system(gs_parse_coord_system(cs));
}


void set_abs_override(bool enable) {mach_set_absolute_mode(enable);}


void set_path_mode(const char *mode) {
  mach_set_path_mode(gs_parse_path_mode(mode));
}


void set_distance_mode(const char *mode) {
  mach_set_distance_mode(gs_parse_distance_mode(mode));
}


void set_arc_dist_mode(const char *mode) {
  mach_set_arc_distance_mode(gs_parse_distance_mode(mode));
}


void set_feed_override(float value) {mach_set_feed_override(value);}
void set_speed_override(float value) {mach_set_spindle_override(value);}
void set_mist_coolant(bool enable) {coolant_set_mist(enable);}
void set_flood_coolant(bool enable) {coolant_set_flood(enable);}


// System
float get_velocity() {return mp_runtime_get_velocity();}
bool get_echo() {return usart_is_set(USART_ECHO);}
void set_echo(bool value) {return usart_set(USART_ECHO, value);}
PGM_P get_state() {return mp_get_state_pgmstr(mp_get_state());}
PGM_P get_cycle() {return mp_get_cycle_pgmstr(mp_get_cycle());}


PGM_P get_hold_reason() {
  return mp_get_hold_reason_pgmstr(mp_get_hold_reason());
}
