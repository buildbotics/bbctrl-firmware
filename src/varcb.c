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

#include "usart.h"
#include "machine.h"
#include "plan/runtime.h"
#include "plan/state.h"

// Axis
float get_position(int index) {return mp_runtime_get_axis_position(index);}

// GCode
int32_t get_line() {return mp_runtime_get_line();}
PGM_P get_unit() {return mp_get_units_mode_pgmstr(mach_get_units_mode());}
float get_speed() {return mach_get_spindle_speed();}
float get_feed() {return mach_get_feed_rate();}
uint8_t get_tool() {return mach_get_tool();}


PGM_P get_feed_rate_mode() {
  return mp_get_feed_rate_mode_pgmstr(mach.gm.feed_rate_mode);
}


PGM_P get_plane() {return mp_get_plane_pgmstr(mach.gm.plane);}


PGM_P get_coord_system() {
  return mp_get_coord_system_pgmstr(mach.gm.coord_system);
}


bool get_abs_override() {return mach.gm.absolute_mode;}
PGM_P get_path_control() {return mp_get_path_mode_pgmstr(mach.gm.path_control);}


PGM_P get_distance_mode() {
  return mp_get_distance_mode_pgmstr(mach.gm.distance_mode);
}


PGM_P get_arc_dist_mode() {
  return mp_get_distance_mode_pgmstr(mach.gm.arc_distance_mode);
}


float get_feed_override() {
  return mach.gm.feed_override_enable ? mach.gm.feed_override_factor : 0;
}


float get_speed_override() {
  return mach.gm.spindle_override_enable ? mach.gm.spindle_override_factor : 0;
}


bool get_mist_coolant() {return mach.gm.mist_coolant;}
bool get_flood_coolant() {return mach.gm.flood_coolant;}

// System
float get_velocity() {return mp_runtime_get_velocity();}
bool get_echo() {return usart_is_set(USART_ECHO);}
void set_echo(bool value) {return usart_set(USART_ECHO, value);}
PGM_P get_state() {return mp_get_state_pgmstr(mp_get_state());}
PGM_P get_cycle() {return mp_get_cycle_pgmstr(mp_get_cycle());}
