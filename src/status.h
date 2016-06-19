/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include <avr/pgmspace.h>


// ritorno is a handy way to provide exception returns
// It returns only if an error occurred. (ritorno is Italian for return)
#define ritorno(a) if ((status_code = a) != STAT_OK) {return status_code;}

typedef enum {
  // OS, communications and low-level status
  STAT_OK,                        // function completed OK
  STAT_EAGAIN,                    // function would block (call again)
  STAT_NOOP,                      // function had no-operation
  STAT_COMPLETE,                  // operation is complete
  STAT_NO_SUCH_DEVICE,
  STAT_BUFFER_FULL,
  STAT_BUFFER_FULL_FATAL,
  STAT_EEPROM_DATA_INVALID,
  STAT_MOTOR_ERROR,
  STAT_INTERNAL_ERROR,            // unrecoverable internal error

  STAT_PREP_LINE_MOVE_TIME_IS_INFINITE,
  STAT_PREP_LINE_MOVE_TIME_IS_NAN,

  // Generic data input errors
  STAT_UNRECOGNIZED_NAME,
  STAT_INVALID_OR_MALFORMED_COMMAND,
  STAT_BAD_NUMBER_FORMAT,
  STAT_PARAMETER_IS_READ_ONLY,
  STAT_PARAMETER_CANNOT_BE_READ,
  STAT_COMMAND_NOT_ACCEPTED,
  STAT_INPUT_EXCEEDS_MAX_LENGTH,
  STAT_INPUT_LESS_THAN_MIN_VALUE,
  STAT_INPUT_EXCEEDS_MAX_VALUE,
  STAT_INPUT_VALUE_RANGE_ERROR,

  // Gcode errors & warnings (Most originate from NIST)
  // Fascinating: http://www.cncalarms.com/
  STAT_GCODE_COMMAND_UNSUPPORTED,
  STAT_MCODE_COMMAND_UNSUPPORTED,
  STAT_GCODE_AXIS_IS_MISSING,
  STAT_GCODE_FEEDRATE_NOT_SPECIFIED,
  STAT_ARC_SPECIFICATION_ERROR,
  STAT_ARC_AXIS_MISSING_FOR_SELECTED_PLANE,
  STAT_ARC_RADIUS_OUT_OF_TOLERANCE,
  STAT_ARC_ENDPOINT_IS_STARTING_POINT,

  // Errors and warnings
  STAT_MINIMUM_LENGTH_MOVE,         // move is less than minimum length
  STAT_MINIMUM_TIME_MOVE,           // move is less than minimum time
  STAT_MACHINE_ALARMED,             // machine is alarmed
  STAT_LIMIT_SWITCH_HIT,            // a limit switch was hit

  STAT_SOFT_LIMIT_EXCEEDED,

  STAT_HOMING_CYCLE_FAILED,         // homing cycle did not complete
  STAT_HOMING_ERROR_BAD_OR_NO_AXIS,
  STAT_HOMING_ERROR_ZERO_SEARCH_VELOCITY,
  STAT_HOMING_ERROR_ZERO_LATCH_VELOCITY,
  STAT_HOMING_ERROR_TRAVEL_MIN_MAX_IDENTICAL,
  STAT_HOMING_ERROR_NEGATIVE_LATCH_BACKOFF,
  STAT_HOMING_ERROR_SWITCH_MISCONFIGURATION,

  STAT_PROBE_CYCLE_FAILED,          // probing cycle did not complete
  // Do not exceed 255
} stat_t;


extern stat_t status_code;

const char *status_to_pgmstr(stat_t status);
void status_error_P(const char *location, const char *msg, stat_t status);

#define TO_STRING(x) _TO_STRING(x)
#define _TO_STRING(x) #x

#define STATUS_LOCATION PSTR(__FILE__ ":" TO_STRING(__LINE__))
#define STATUS_ERROR(MSG, CODE) status_error_P(STATUS_LOCATION, PSTR(MSG), CODE)
