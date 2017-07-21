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


static const char INVALID_PGMSTR[] PROGMEM = "INVALID";

static const char INCHES_PGMSTR[]      PROGMEM = "IN";
static const char MILLIMETERS_PGMSTR[] PROGMEM = "MM";
static const char DEGREES_PGMSTR[]     PROGMEM = "DEG";

static const char INVERSE_TIME_MODE_PGMSTR[] PROGMEM         = "INVERSE TIME";
static const char UNITS_PER_MINUTE_MODE_PGMSTR[] PROGMEM     = "PER MIN";
static const char UNITS_PER_REVOLUTION_MODE_PGMSTR[] PROGMEM = "PER REV";

static const char PLANE_XY_PGMSTR[] PROGMEM = "XY";
static const char PLANE_XZ_PGMSTR[] PROGMEM = "XZ";
static const char PLANE_YZ_PGMSTR[] PROGMEM = "YZ";

static const char ABSOLUTE_COORDS_PGMSTR[] PROGMEM = "ABS";
static const char G54_PGMSTR[] PROGMEM = "G54";
static const char G55_PGMSTR[] PROGMEM = "G55";
static const char G56_PGMSTR[] PROGMEM = "G56";
static const char G57_PGMSTR[] PROGMEM = "G57";
static const char G58_PGMSTR[] PROGMEM = "G58";
static const char G59_PGMSTR[] PROGMEM = "G59";

static const char PATH_EXACT_PATH_PGMSTR[] PROGMEM = "EXACT PATH";
static const char PATH_EXACT_STOP_PGMSTR[] PROGMEM = "EXACT STOP";
static const char PATH_CONTINUOUS_PGMSTR[] PROGMEM = "CONTINUOUS";

static const char ABSOLUTE_MODE_PGMSTR[] PROGMEM    = "ABSOLUTE";
static const char INCREMENTAL_MODE_PGMSTR[] PROGMEM = "INCREMENTAL";


PGM_P gs_get_units_pgmstr(units_t mode) {
  switch (mode) {
  case INCHES:      return INCHES_PGMSTR;
  case MILLIMETERS: return MILLIMETERS_PGMSTR;
  case DEGREES:     return DEGREES_PGMSTR;
  }

  return INVALID_PGMSTR;
}


units_t gs_parse_units(const char *s) {
  if (!strcmp_P(s, INCHES_PGMSTR)) return INCHES;
  if (!strcmp_P(s, MILLIMETERS_PGMSTR)) return MILLIMETERS;
  if (!strcmp_P(s, DEGREES_PGMSTR)) return DEGREES;
  return -1;
}


PGM_P gs_get_feed_mode_pgmstr(feed_mode_t mode) {
  switch (mode) {
  case INVERSE_TIME_MODE:         return INVERSE_TIME_MODE_PGMSTR;
  case UNITS_PER_MINUTE_MODE:     return UNITS_PER_MINUTE_MODE_PGMSTR;
  case UNITS_PER_REVOLUTION_MODE: return UNITS_PER_REVOLUTION_MODE_PGMSTR;
  }

  return INVALID_PGMSTR;
}


feed_mode_t gs_parse_feed_mode(const char *s) {
  if (!strcmp_P(s, INVERSE_TIME_MODE_PGMSTR)) return INVERSE_TIME_MODE;
  if (!strcmp_P(s, UNITS_PER_MINUTE_MODE_PGMSTR)) return UNITS_PER_MINUTE_MODE;
  if (!strcmp_P(s, UNITS_PER_REVOLUTION_MODE_PGMSTR))
    return UNITS_PER_REVOLUTION_MODE;
  return -1;
}


PGM_P gs_get_plane_pgmstr(plane_t plane) {
  switch (plane) {
  case PLANE_XY: return PLANE_XY_PGMSTR;
  case PLANE_XZ: return PLANE_XZ_PGMSTR;
  case PLANE_YZ: return PLANE_YZ_PGMSTR;
  }

  return INVALID_PGMSTR;
}


plane_t gs_parse_plane(const char *s) {
  if (!strcmp_P(s, PLANE_XY_PGMSTR)) return PLANE_XY;
  if (!strcmp_P(s, PLANE_XZ_PGMSTR)) return PLANE_XZ;
  if (!strcmp_P(s, PLANE_YZ_PGMSTR)) return PLANE_YZ;
  return -1;
}


PGM_P gs_get_coord_system_pgmstr(coord_system_t cs) {
  switch (cs) {
  case ABSOLUTE_COORDS: return ABSOLUTE_COORDS_PGMSTR;
  case G54: return G54_PGMSTR;
  case G55: return G55_PGMSTR;
  case G56: return G56_PGMSTR;
  case G57: return G57_PGMSTR;
  case G58: return G58_PGMSTR;
  case G59: return G59_PGMSTR;
  }

  return INVALID_PGMSTR;
}


coord_system_t gs_parse_coord_system(const char *s) {
  if (!strcmp_P(s, ABSOLUTE_COORDS_PGMSTR)) return ABSOLUTE_COORDS;
  if (!strcmp_P(s, G54_PGMSTR)) return G54;
  if (!strcmp_P(s, G55_PGMSTR)) return G55;
  if (!strcmp_P(s, G56_PGMSTR)) return G56;
  if (!strcmp_P(s, G57_PGMSTR)) return G57;
  if (!strcmp_P(s, G58_PGMSTR)) return G58;
  if (!strcmp_P(s, G59_PGMSTR)) return G59;
  return -1;
}


PGM_P gs_get_path_mode_pgmstr(path_mode_t mode) {
  switch (mode) {
  case PATH_EXACT_PATH: return PATH_EXACT_PATH_PGMSTR;
  case PATH_EXACT_STOP: return PATH_EXACT_STOP_PGMSTR;
  case PATH_CONTINUOUS: return PATH_CONTINUOUS_PGMSTR;
  }

  return INVALID_PGMSTR;
}


path_mode_t gs_parse_path_mode(const char *s) {
  if (!strcmp_P(s, PATH_EXACT_PATH_PGMSTR)) return PATH_EXACT_PATH;
  if (!strcmp_P(s, PATH_EXACT_STOP_PGMSTR)) return PATH_EXACT_STOP;
  if (!strcmp_P(s, PATH_CONTINUOUS_PGMSTR)) return PATH_CONTINUOUS;
  return -1;
}


PGM_P gs_get_distance_mode_pgmstr(distance_mode_t mode) {
  switch (mode) {
  case ABSOLUTE_MODE:    return ABSOLUTE_MODE_PGMSTR;
  case INCREMENTAL_MODE: return INCREMENTAL_MODE_PGMSTR;
  }

  return INVALID_PGMSTR;
}


distance_mode_t gs_parse_distance_mode(const char *s) {
  if (!strcmp_P(s, ABSOLUTE_MODE_PGMSTR)) return ABSOLUTE_MODE;
  if (!strcmp_P(s, INCREMENTAL_MODE_PGMSTR)) return INCREMENTAL_MODE;
  return -1;
}
