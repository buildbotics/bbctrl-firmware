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


/* Status codes
 *
 * Status codes are divided into ranges for clarity and extensibility. At some
 * point this may break down and the whole thing will get messy(er), but it's
 * advised not to change the values of existing status codes once they are in
 * distribution.
 *
 * Ranges are:
 *
 *   0 - 19    OS, communications and low-level status
 *
 *  20 - 99    Generic internal and application errors. Internal errors start at
 *             20 and work up, Assertion failures start at 99 and work down.
 *
 * 100 - 129   Generic data and input errors - not specific to Gcode
 *
 * 130 -       Gcode and application errors and warnings
 *
 * See main.c for associated message strings. Any changes to the codes may also
 * require changing the message strings and string array in main.c
 *
 * Most of the status codes (except STAT_OK) below are errors which would fail
 * the command, and are returned by the failed command and reported back via
 * JSON or text. Some status codes are warnings do not fail the command. These
 * can be used to generate an exception report. These are labeled as WARNING
 */

#include <stdint.h>

typedef uint8_t stat_t;
extern stat_t status_code;

void print_status_message(const char *msg, stat_t status);

// ritorno is a handy way to provide exception returns
// It returns only if an error occurred. (ritorno is Italian for return)
#define ritorno(a) if ((status_code = a) != STAT_OK) {return status_code;}

// OS, communications and low-level status
#define STAT_OK 0                        // function completed OK
#define STAT_ERROR 1                     // generic error return EPERM
#define STAT_EAGAIN 2                    // function would block (call again)
#define STAT_NOOP 3                      // function had no-operation
#define STAT_COMPLETE 4                  // operation is complete
#define STAT_TERMINATE 5                 // operation terminated (gracefully)
#define STAT_RESET 6                     // operation was hard reset (sig kill)
#define STAT_EOL 7                       // function returned end-of-line
#define STAT_EOF 8                       // function returned end-of-file
#define STAT_FILE_NOT_OPEN 9
#define STAT_FILE_SIZE_EXCEEDED 10
#define STAT_NO_SUCH_DEVICE 11
#define STAT_BUFFER_EMPTY 12
#define STAT_BUFFER_FULL 13
#define STAT_BUFFER_FULL_FATAL 14
#define STAT_INITIALIZING 15              // initializing - not ready for use
#define STAT_ENTERING_BOOT_LOADER 16      // emitted from boot loader
#define STAT_FUNCTION_IS_STUBBED 17
#define STAT_EEPROM_DATA_INVALID 18

// Internal errors and startup messages
#define STAT_INTERNAL_ERROR 20            // unrecoverable internal error
#define STAT_INTERNAL_RANGE_ERROR 21      // other than by user input
#define STAT_FLOATING_POINT_ERROR 22      // number conversion error
#define STAT_DIVIDE_BY_ZERO 23
#define STAT_INVALID_ADDRESS 24
#define STAT_READ_ONLY_ADDRESS 25
#define STAT_INIT_FAIL 26
#define STAT_ALARMED 27
#define STAT_FAILED_TO_GET_PLANNER_BUFFER 28
#define STAT_GENERIC_EXCEPTION_REPORT 29    // used for test

#define STAT_PREP_LINE_MOVE_TIME_IS_INFINITE 30
#define STAT_PREP_LINE_MOVE_TIME_IS_NAN 31
#define STAT_FLOAT_IS_INFINITE 32
#define STAT_FLOAT_IS_NAN 33
#define STAT_PERSISTENCE_ERROR 34
#define STAT_BAD_STATUS_REPORT_SETTING 35

// Assertion failures - down from 99 until they meet system internal errors
#define STAT_CONFIG_ASSERTION_FAILURE 90
#define STAT_ENCODER_ASSERTION_FAILURE 92
#define STAT_STEPPER_ASSERTION_FAILURE 93
#define STAT_PLANNER_ASSERTION_FAILURE 94
#define STAT_CANONICAL_MACHINE_ASSERTION_FAILURE 95
#define STAT_CONTROLLER_ASSERTION_FAILURE 96
#define STAT_STACK_OVERFLOW 97
#define STAT_MEMORY_FAULT 98
#define STAT_GENERIC_ASSERTION_FAILURE 99

// Application and data input errors

// Generic data input errors
#define STAT_UNRECOGNIZED_NAME 100
#define STAT_INVALID_OR_MALFORMED_COMMAND 101
#define STAT_BAD_NUMBER_FORMAT 102
#define STAT_UNSUPPORTED_TYPE 103
#define STAT_PARAMETER_IS_READ_ONLY 104
#define STAT_PARAMETER_CANNOT_BE_READ 105
#define STAT_COMMAND_NOT_ACCEPTED 106
#define STAT_INPUT_EXCEEDS_MAX_LENGTH 107
#define STAT_INPUT_LESS_THAN_MIN_VALUE 108
#define STAT_INPUT_EXCEEDS_MAX_VALUE 109

#define STAT_INPUT_VALUE_RANGE_ERROR 110
#define STAT_JSON_SYNTAX_ERROR 111
#define STAT_JSON_TOO_MANY_PAIRS 112
#define STAT_JSON_TOO_LONG 113

// Gcode errors and warnings (Most originate from NIST - by concept, not number)
// Fascinating: http://www.cncalarms.com/
#define STAT_GCODE_GENERIC_INPUT_ERROR 130
#define STAT_GCODE_COMMAND_UNSUPPORTED 131
#define STAT_MCODE_COMMAND_UNSUPPORTED 132
#define STAT_GCODE_MODAL_GROUP_VIOLATION 133
#define STAT_GCODE_AXIS_IS_MISSING 134
#define STAT_GCODE_AXIS_CANNOT_BE_PRESENT 135
#define STAT_GCODE_AXIS_IS_INVALID 136
#define STAT_GCODE_AXIS_IS_NOT_CONFIGURED 137
#define STAT_GCODE_AXIS_NUMBER_IS_MISSING 138
#define STAT_GCODE_AXIS_NUMBER_IS_INVALID 139

#define STAT_GCODE_ACTIVE_PLANE_IS_MISSING 140
#define STAT_GCODE_ACTIVE_PLANE_IS_INVALID 141
#define STAT_GCODE_FEEDRATE_NOT_SPECIFIED 142
#define STAT_GCODE_INVERSE_TIME_MODE_CANNOT_BE_USED 143
#define STAT_GCODE_ROTARY_AXIS_CANNOT_BE_USED 144
#define STAT_GCODE_G53_WITHOUT_G0_OR_G1 145
#define STAT_REQUESTED_VELOCITY_EXCEEDS_LIMITS 146
#define STAT_CUTTER_COMPENSATION_CANNOT_BE_ENABLED 147
#define STAT_PROGRAMMED_POINT_SAME_AS_CURRENT_POINT 148
#define STAT_SPINDLE_SPEED_BELOW_MINIMUM 149

#define STAT_SPINDLE_SPEED_MAX_EXCEEDED 150
#define STAT_S_WORD_IS_MISSING 151
#define STAT_S_WORD_IS_INVALID 152
#define STAT_SPINDLE_MUST_BE_OFF 153
#define STAT_SPINDLE_MUST_BE_TURNING 154
#define STAT_ARC_SPECIFICATION_ERROR 155
#define STAT_ARC_AXIS_MISSING_FOR_SELECTED_PLANE 156
#define STAT_ARC_OFFSETS_MISSING_FOR_SELECTED_PLANE 157
#define STAT_ARC_RADIUS_OUT_OF_TOLERANCE 158
#define STAT_ARC_ENDPOINT_IS_STARTING_POINT 159

#define STAT_P_WORD_IS_MISSING 160
#define STAT_P_WORD_IS_INVALID 161
#define STAT_P_WORD_IS_ZERO 162
#define STAT_P_WORD_IS_NEGATIVE 163
#define STAT_P_WORD_IS_NOT_AN_INTEGER 164
#define STAT_P_WORD_IS_NOT_VALID_TOOL_NUMBER 165
#define STAT_D_WORD_IS_MISSING 166
#define STAT_D_WORD_IS_INVALID 167
#define STAT_E_WORD_IS_MISSING 168
#define STAT_E_WORD_IS_INVALID 169

#define STAT_H_WORD_IS_MISSING 170
#define STAT_H_WORD_IS_INVALID 171
#define STAT_L_WORD_IS_MISSING 172
#define STAT_L_WORD_IS_INVALID 173
#define STAT_Q_WORD_IS_MISSING 174
#define STAT_Q_WORD_IS_INVALID 175
#define STAT_R_WORD_IS_MISSING 176
#define STAT_R_WORD_IS_INVALID 177
#define STAT_T_WORD_IS_MISSING 178
#define STAT_T_WORD_IS_INVALID 179

// Errors and warnings
#define STAT_GENERIC_ERROR 200
#define STAT_MINIMUM_LENGTH_MOVE 201         // move is less than minimum length
#define STAT_MINIMUM_TIME_MOVE 202           // move is less than minimum time
#define STAT_MACHINE_ALARMED 203             // machine is alarmed
#define STAT_LIMIT_SWITCH_HIT 204            // a limit switch was hit
#define STAT_PLANNER_FAILED_TO_CONVERGE 205  // trapezoid generator throws this

#define STAT_SOFT_LIMIT_EXCEEDED 220                    // axis unspecified
#define STAT_SOFT_LIMIT_EXCEEDED_XMIN 221
#define STAT_SOFT_LIMIT_EXCEEDED_XMAX 222
#define STAT_SOFT_LIMIT_EXCEEDED_YMIN 223
#define STAT_SOFT_LIMIT_EXCEEDED_YMAX 224
#define STAT_SOFT_LIMIT_EXCEEDED_ZMIN 225
#define STAT_SOFT_LIMIT_EXCEEDED_ZMAX 226
#define STAT_SOFT_LIMIT_EXCEEDED_AMIN 227
#define STAT_SOFT_LIMIT_EXCEEDED_AMAX 228
#define STAT_SOFT_LIMIT_EXCEEDED_BMIN 229
#define STAT_SOFT_LIMIT_EXCEEDED_BMAX 220
#define STAT_SOFT_LIMIT_EXCEEDED_CMIN 231
#define STAT_SOFT_LIMIT_EXCEEDED_CMAX 232

#define STAT_HOMING_CYCLE_FAILED 240           // homing cycle did not complete
#define STAT_HOMING_ERROR_BAD_OR_NO_AXIS 241
#define STAT_HOMING_ERROR_ZERO_SEARCH_VELOCITY 242
#define STAT_HOMING_ERROR_ZERO_LATCH_VELOCITY 243
#define STAT_HOMING_ERROR_TRAVEL_MIN_MAX_IDENTICAL 244
#define STAT_HOMING_ERROR_NEGATIVE_LATCH_BACKOFF 245
#define STAT_HOMING_ERROR_SWITCH_MISCONFIGURATION 246

#define STAT_PROBE_CYCLE_FAILED 250            // probing cycle did not complete
#define STAT_PROBE_ENDPOINT_IS_STARTING_POINT 251

// Do not exceed 255 without also changing stat_t typedef
