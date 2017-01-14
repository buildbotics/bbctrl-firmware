/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2017 Buildbotics LLC
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

#include "gcode_state.h"


PGM_P gs_get_units_pgmstr(units_t mode) {
  switch (mode) {
  case INCHES:      return PSTR("IN");
  case MILLIMETERS: return PSTR("MM");
  case DEGREES:     return PSTR("DEG");
  }

  return PSTR("INVALID");
}


PGM_P gs_get_feed_mode_pgmstr(feed_mode_t mode) {
  switch (mode) {
  case INVERSE_TIME_MODE:         return PSTR("INVERSE TIME");
  case UNITS_PER_MINUTE_MODE:     return PSTR("PER MIN");
  case UNITS_PER_REVOLUTION_MODE: return PSTR("PER REV");
  }

  return PSTR("INVALID");
}


PGM_P gs_get_plane_pgmstr(plane_t plane) {
  switch (plane) {
  case PLANE_XY: return PSTR("XY");
  case PLANE_XZ: return PSTR("XZ");
  case PLANE_YZ: return PSTR("YZ");
  }

  return PSTR("INVALID");
}


PGM_P gs_get_coord_system_pgmstr(coord_system_t cs) {
  switch (cs) {
  case ABSOLUTE_COORDS: return PSTR("ABS");
  case G54: return PSTR("G54");
  case G55: return PSTR("G55");
  case G56: return PSTR("G56");
  case G57: return PSTR("G57");
  case G58: return PSTR("G58");
  case G59: return PSTR("G59");
  }

  return PSTR("INVALID");
}


PGM_P gs_get_path_mode_pgmstr(path_mode_t mode) {
  switch (mode) {
  case PATH_EXACT_PATH: return PSTR("EXACT PATH");
  case PATH_EXACT_STOP: return PSTR("EXACT STOP");
  case PATH_CONTINUOUS: return PSTR("CONTINUOUS");
  }

  return PSTR("INVALID");
}


PGM_P gs_get_distance_mode_pgmstr(distance_mode_t mode) {
  switch (mode) {
  case ABSOLUTE_MODE:    return PSTR("ABSOLUTE");
  case INCREMENTAL_MODE: return PSTR("INCREMENTAL");
  }

  return PSTR("INVALID");
}
