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

#include "gcode_parser.h"

#include "machine.h"
#include "spindle.h"
#include "probing.h"
#include "homing.h"
#include "util.h"

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>


#define SET_MODAL(m, parm, val) \
  {mach.gn.parm = val; mach.gf.parm = 1; modals[m] += 1; break;}
#define SET_NON_MODAL(parm, val) {mach.gn.parm = val; mach.gf.parm = 1; break;}
#define EXEC_FUNC(f, v) if ((uint8_t)mach.gf.v) f(mach.gn.v)


static uint8_t modals[MODAL_GROUP_COUNT]; // collects modal groups in a block


/* Normalize a block (line) of gcode in place
 *
 * Normalization functions:
 *   - convert all letters to upper case
 *   - remove white space, control and other invalid characters
 *   - remove (erroneous) leading zeros that might be taken to mean Octal
 *   - identify and return start of comments and messages
 *   - signal if a block-delete character (/) was encountered in the first space
 *
 * So this: "  g1 x100 Y100 f400" becomes this: "G1X100Y100F400"
 *
 * Comment and message handling:
 *   - Comments field start with a '(' char or alternately a semicolon ';'
 *   - Comments and messages are not normalized - they are left alone
 *   - The 'MSG' specifier in comment can have mixed case but cannot cannot
 *     have embedded white spaces
 *   - Normalization returns true if there was a message to display, false
 *     otherwise
 *   - Comments always terminate the block - i.e. leading or embedded comments
 *     are not supported
 *   - Valid cases (examples)          Notes:
 *     G0X10                           - command only - no comment
 *     (comment text)                  - There is no command on this line
 *     G0X10 (comment text)
 *     G0X10 (comment text             - It's OK to drop the trailing paren
 *     G0X10 ;comment text             - It's OK to drop the trailing paren
 *
 *   - Invalid cases (examples)        Notes:
 *     G0X10 comment text              - Comment with no separator
 *     N10 (comment) G0X10             - embedded comment. G0X10 will be ignored
 *     (comment) G0X10                 - leading comment. G0X10 will be ignored
 *     G0X10 # comment                 - invalid separator
 *
 * Returns:
 *  - com points to comment string or to 0 if no comment
 *  - msg points to message string or to 0 if no comment
 *  - block_delete_flag is set true if block delete encountered, false otherwise
 */

static void _normalize_gcode_block(char *str, char **com, char **msg,
                                   uint8_t *block_delete_flag) {
  char *rd = str; // read pointer
  char *wr = str; // write pointer

  // mark block deletes
  *block_delete_flag = *rd == '/';

  // normalize the command block & find the comment (if any)
  for (; *wr; rd++)
    if (!*rd) *wr = 0;
    else if (*rd == '(' || *rd == ';') {*wr = 0; *com = rd + 1;}
    else if (isalnum(*rd) || strchr("-.", *rd)) // all valid characters
      *wr++ = toupper(*rd);

  // Perform Octal stripping - remove invalid leading zeros in number strings
  rd = str;
  while (*rd) {
    if (*rd == '.') break; // don't strip past a decimal point
    if (!isdigit(*rd) && *(rd + 1) == '0' && isdigit(*(rd + 2))) {
      wr = rd + 1;
      while (*wr) {*wr = *(wr + 1); wr++;}    // copy forward w/overwrite
      continue;
    }

    rd++;
  }

  // process comments and messages
  if (**com) {
    rd = *com;
    while (isspace(*rd)) rd++;        // skip any leading spaces before "msg"

    if (tolower(*rd) == 'm' && tolower(*(rd + 1)) == 's' &&
        tolower(*(rd + 2)) == 'g')
      *msg = rd + 3;

    for (; *rd; rd++)
      // 0 terminate on trailing parenthesis, if any
      if (*rd == ')') *rd = 0;
  }
}


/* Get gcode word consisting of a letter and a value
 *
 * This function requires the Gcode string to be normalized.
 * Normalization must remove any leading zeros or they will be converted to
 * octal.  G0X... is not interpreted as hexadecimal. This is trapped.
 */
static stat_t _get_next_gcode_word(char **pstr, char *letter, float *value) {
  if (!**pstr) return STAT_COMPLETE; // no more words to process

  // get letter part
  if (!isupper(**pstr)) return STAT_INVALID_OR_MALFORMED_COMMAND;
  *letter = **pstr;
  (*pstr)++;

  // X-axis-becomes-a-hexadecimal-number get-value case, e.g. G0X100 --> G255
  if (**pstr == '0' && *(*pstr + 1) == 'X') {
    *value = 0;
    (*pstr)++;
    return STAT_OK; // pointer points to X
  }

  // get-value general case
  char *end;
  *value = strtod(*pstr, &end);
  // more robust test then checking for value == 0
  if (end == *pstr) return STAT_GCODE_COMMAND_UNSUPPORTED;
  *pstr = end; // pointer points to next character after the word

  return STAT_OK;
}


/// Isolate the decimal point value as an integer
static uint8_t _point(float value) {
  return value * 10 - trunc(value) * 10;
}


/// Check for some gross Gcode block semantic violations
static stat_t _validate_gcode_block() {
  // Check for modal group violations. From NIST, section 3.4 "It
  // is an error to put a G-code from group 1 and a G-code from
  // group 0 on the same line if both of them use axis words. If an
  // axis word-using G-code from group 1 is implicitly in effect on
  // a line (by having been activated on an earlier line), and a
  // group 0 G-code that uses axis words appears on the line, the
  // activity of the group 1 G-code is suspended for that line. The
  // axis word-using G-codes from group 0 are G10, G28, G30, and G92"

  // if (modals[MODAL_GROUP_G0] && modals[MODAL_GROUP_G1])
  //   return STAT_MODAL_GROUP_VIOLATION;

  // look for commands that require an axis word to be present
  // if (modals[MODAL_GROUP_G0] || modals[MODAL_GROUP_G1])
  //   if (!_axis_changed()) return STAT_GCODE_AXIS_IS_MISSING;

  return STAT_OK;
}


/* Execute parsed block
 *
 * Conditionally (based on whether a flag is set in gf) call the
 * machining functions in order of execution as per RS274NGC_3 table 8
 * (below, with modifications):
 *
 *   0. record the line number
 *   1. comment (includes message) [handled during block normalization]
 *   2. set feed rate mode (G93, G94 - inverse time or per minute)
 *   3. set feed rate (F)
 *   3a. set feed override rate (M50)
 *   4. set spindle speed (S)
 *   4a. set spindle override rate (M51.1)
 *   5. select tool (T)
 *   6. change tool (M6)
 *   7. spindle on or off (M3, M4, M5)
 *   8. coolant on or off (M7, M8, M9)
 *   9. enable or disable overrides (M48, M49, M50, M51)
 *   10. dwell (G4)
 *   11. set active plane (G17, G18, G19)
 *   12. set length units (G20, G21)
 *   13. cutter radius compensation on or off (G40, G41, G42)
 *   14. cutter length compensation on or off (G43, G49)
 *   15. coordinate system selection (G54, G55, G56, G57, G58, G59)
 *   16. set path control mode (G61, G61.1, G64)
 *   17. set distance mode (G90, G91)
 *   18. set retract mode (G98, G99)
 *   19a. homing functions (G28.2, G28.3, G28.1, G28, G30)
 *   19b. update system data (G10)
 *   19c. set axis offsets (G92, G92.1, G92.2, G92.3)
 *   20. perform motion (G0 to G3, G80-G89) as modified (possibly) by G53
 *   21. stop and end (M0, M1, M2, M30, M60)
 *
 * Values in gn are in original units and should not be unit converted prior
 * to calling the machine functions (which do the unit conversions)
 */
static stat_t _execute_gcode_block() {
  stat_t status = STAT_OK;

  mach_set_model_line(mach.gn.line);
  EXEC_FUNC(mach_set_feed_rate_mode, feed_rate_mode);
  EXEC_FUNC(mach_set_feed_rate, feed_rate);
  EXEC_FUNC(mach_feed_override_factor, feed_override_factor);
  EXEC_FUNC(mach_set_spindle_speed, spindle_speed);
  EXEC_FUNC(mach_spindle_override_factor, spindle_override_factor);
  EXEC_FUNC(mach_select_tool, tool_select);
  EXEC_FUNC(mach_change_tool, tool_change);
  EXEC_FUNC(mach_set_spindle_mode, spindle_mode);
  EXEC_FUNC(mach_mist_coolant_control, mist_coolant);
  EXEC_FUNC(mach_flood_coolant_control, flood_coolant);
  EXEC_FUNC(mach_feed_override_enable, feed_override_enable);
  EXEC_FUNC(mach_spindle_override_enable, spindle_override_enable);
  EXEC_FUNC(mach_override_enables, override_enables);

  if (mach.gn.next_action == NEXT_ACTION_DWELL) // G4 - dwell
    RITORNO(mach_dwell(mach.gn.parameter));

  EXEC_FUNC(mach_set_plane, plane);
  EXEC_FUNC(mach_set_units_mode, units_mode);
  //--> cutter radius compensation goes here
  //--> cutter length compensation goes here
  EXEC_FUNC(mach_set_coord_system, coord_system);
  EXEC_FUNC(mach_set_path_control, path_control);
  EXEC_FUNC(mach_set_distance_mode, distance_mode);
  //--> set retract mode goes here

  switch (mach.gn.next_action) {
  case NEXT_ACTION_SET_G28_POSITION: // G28.1
    mach_set_g28_position();
    break;
  case NEXT_ACTION_GOTO_G28_POSITION: // G28
    status = mach_goto_g28_position(mach.gn.target, mach.gf.target);
    break;
  case NEXT_ACTION_SET_G30_POSITION: // G30.1
    mach_set_g30_position();
    break;
  case NEXT_ACTION_GOTO_G30_POSITION: // G30
    status = mach_goto_g30_position(mach.gn.target, mach.gf.target);
    break;
  case NEXT_ACTION_SEARCH_HOME: // G28.2
    mach_homing_cycle_start();
    break;
  case NEXT_ACTION_SET_ABSOLUTE_ORIGIN: // G28.3
    mach_set_absolute_origin(mach.gn.target, mach.gf.target);
    break;
  case NEXT_ACTION_HOMING_NO_SET: // G28.4
    mach_homing_cycle_start_no_set();
    break;
  case NEXT_ACTION_STRAIGHT_PROBE: // G38.2
    status = mach_probe(mach.gn.target, mach.gf.target);
    break;
  case NEXT_ACTION_SET_COORD_DATA:
    mach_set_coord_offsets(mach.gn.parameter, mach.gn.target, mach.gf.target);
    break;
  case NEXT_ACTION_SET_ORIGIN_OFFSETS:
    mach_set_origin_offsets(mach.gn.target, mach.gf.target);
    break;
  case NEXT_ACTION_RESET_ORIGIN_OFFSETS:
    mach_reset_origin_offsets();
    break;
  case NEXT_ACTION_SUSPEND_ORIGIN_OFFSETS:
    mach_suspend_origin_offsets();
    break;
  case NEXT_ACTION_RESUME_ORIGIN_OFFSETS:
    mach_resume_origin_offsets();
    break;
  case NEXT_ACTION_DWELL: break; // Handled above

  case NEXT_ACTION_DEFAULT:
    // apply override setting to gm struct
    mach_set_absolute_mode(mach.gn.absolute_mode);

    switch (mach.gn.motion_mode) {
    case MOTION_MODE_CANCEL_MOTION_MODE:
      mach.gm.motion_mode = mach.gn.motion_mode;
      break;
    case MOTION_MODE_RAPID:
      status = mach_rapid(mach.gn.target, mach.gf.target);
      break;
    case MOTION_MODE_FEED:
      status = mach_feed(mach.gn.target, mach.gf.target);
      break;
    case MOTION_MODE_CW_ARC: case MOTION_MODE_CCW_ARC:
      // gf.radius sets radius mode if radius was collected in gn
      status = mach_arc_feed(mach.gn.target, mach.gf.target,
                             mach.gn.arc_offset[0], mach.gn.arc_offset[1],
                             mach.gn.arc_offset[2], mach.gn.arc_radius,
                             mach.gn.motion_mode);
      break;
    default: break; // Should not get here
    }
  }
  // un-set absolute override once the move is planned
  mach_set_absolute_mode(false);

  // do the program stops and ends : M0, M1, M2, M30, M60
  if (mach.gf.program_flow)
    switch (mach.gn.program_flow) {
    case PROGRAM_STOP: mach_program_stop(); break;
    case PROGRAM_OPTIONAL_STOP: mach_optional_program_stop(); break;
    case PROGRAM_PALLET_CHANGE_STOP: mach_pallet_change_stop(); break;
    case PROGRAM_END: mach_program_end(); break;
    }

  return status;
}


/* Parses one line of 0 terminated G-Code.
 *
 * All the parser does is load the state values in gn (next model
 * state) and set flags in gf (model state flags). The execute
 * routine applies them. The buffer is assumed to contain only
 * uppercase characters and signed floats (no whitespace).
 *
 * A number of implicit things happen when the gn struct is zeroed:
 *   - inverse feed rate mode is canceled - set back to units_per_minute mode
 */
static stat_t _parse_gcode_block(char *buf) {
  char *pstr = buf;         // persistent pointer for parsing words
  char letter;              // parsed letter, eg.g. G or X or Y
  float value = 0;          // value parsed from letter (e.g. 2 for G2)
  stat_t status = STAT_OK;

  // set initial state for new move
  memset(modals, 0, sizeof(modals));              // clear all parser values
  memset(&mach.gf, 0, sizeof(gcode_state_t));     // clear all next-state flags
  memset(&mach.gn, 0, sizeof(gcode_state_t));     // clear all next-state values

  // get motion mode from previous block
  mach.gn.motion_mode = mach_get_motion_mode();

  // extract commands and parameters
  while ((status = _get_next_gcode_word(&pstr, &letter, &value)) == STAT_OK) {
    switch (letter) {
    case 'G':
      switch ((uint8_t)value) {
      case 0:
        SET_MODAL(MODAL_GROUP_G1, motion_mode, MOTION_MODE_RAPID);
      case 1:
        SET_MODAL(MODAL_GROUP_G1, motion_mode, MOTION_MODE_FEED);
      case 2: SET_MODAL(MODAL_GROUP_G1, motion_mode, MOTION_MODE_CW_ARC);
      case 3: SET_MODAL(MODAL_GROUP_G1, motion_mode, MOTION_MODE_CCW_ARC);
      case 4: SET_NON_MODAL(next_action, NEXT_ACTION_DWELL);
      case 10:
        SET_MODAL(MODAL_GROUP_G0, next_action, NEXT_ACTION_SET_COORD_DATA);
      case 17: SET_MODAL(MODAL_GROUP_G2, plane, PLANE_XY);
      case 18: SET_MODAL(MODAL_GROUP_G2, plane, PLANE_XZ);
      case 19: SET_MODAL(MODAL_GROUP_G2, plane, PLANE_YZ);
      case 20: SET_MODAL(MODAL_GROUP_G6, units_mode, INCHES);
      case 21: SET_MODAL(MODAL_GROUP_G6, units_mode, MILLIMETERS);
      case 28:
        switch (_point(value)) {
        case 0:
          SET_MODAL(MODAL_GROUP_G0, next_action, NEXT_ACTION_GOTO_G28_POSITION);
        case 1:
          SET_MODAL(MODAL_GROUP_G0, next_action, NEXT_ACTION_SET_G28_POSITION);
        case 2: SET_NON_MODAL(next_action, NEXT_ACTION_SEARCH_HOME);
        case 3: SET_NON_MODAL(next_action, NEXT_ACTION_SET_ABSOLUTE_ORIGIN);
        case 4: SET_NON_MODAL(next_action, NEXT_ACTION_HOMING_NO_SET);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 30:
        switch (_point(value)) {
        case 0:
          SET_MODAL(MODAL_GROUP_G0, next_action, NEXT_ACTION_GOTO_G30_POSITION);
        case 1:
          SET_MODAL(MODAL_GROUP_G0, next_action, NEXT_ACTION_SET_G30_POSITION);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 38:
        switch (_point(value)) {
        case 2: SET_NON_MODAL(next_action, NEXT_ACTION_STRAIGHT_PROBE);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 40: break; // ignore cancel cutter radius compensation
      case 49: break; // ignore cancel tool length offset comp.
      case 53: SET_NON_MODAL(absolute_mode, true);
      case 54: SET_MODAL(MODAL_GROUP_G12, coord_system, G54);
      case 55: SET_MODAL(MODAL_GROUP_G12, coord_system, G55);
      case 56: SET_MODAL(MODAL_GROUP_G12, coord_system, G56);
      case 57: SET_MODAL(MODAL_GROUP_G12, coord_system, G57);
      case 58: SET_MODAL(MODAL_GROUP_G12, coord_system, G58);
      case 59: SET_MODAL(MODAL_GROUP_G12, coord_system, G59);
      case 61:
        switch (_point(value)) {
        case 0: SET_MODAL(MODAL_GROUP_G13, path_control, PATH_EXACT_PATH);
        case 1: SET_MODAL(MODAL_GROUP_G13, path_control, PATH_EXACT_STOP);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 64: SET_MODAL(MODAL_GROUP_G13,path_control, PATH_CONTINUOUS);
      case 80: SET_MODAL(MODAL_GROUP_G1, motion_mode,
                         MOTION_MODE_CANCEL_MOTION_MODE);
        // case 90: SET_MODAL(MODAL_GROUP_G3, distance_mode, ABSOLUTE_MODE);
        // case 91: SET_MODAL(MODAL_GROUP_G3, distance_mode, INCREMENTAL_MODE);
      case 90:
        switch (_point(value)) {
        case 0: SET_MODAL(MODAL_GROUP_G3, distance_mode, ABSOLUTE_MODE);
        case 1: SET_MODAL(MODAL_GROUP_G3, arc_distance_mode, ABSOLUTE_MODE);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 91:
        switch (_point(value)) {
        case 0: SET_MODAL(MODAL_GROUP_G3, distance_mode, INCREMENTAL_MODE);
        case 1: SET_MODAL(MODAL_GROUP_G3, arc_distance_mode, INCREMENTAL_MODE);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 92:
        switch (_point(value)) {
        case 0: SET_MODAL(MODAL_GROUP_G0, next_action,
                          NEXT_ACTION_SET_ORIGIN_OFFSETS);
        case 1: SET_NON_MODAL(next_action, NEXT_ACTION_RESET_ORIGIN_OFFSETS);
        case 2: SET_NON_MODAL(next_action, NEXT_ACTION_SUSPEND_ORIGIN_OFFSETS);
        case 3: SET_NON_MODAL(next_action, NEXT_ACTION_RESUME_ORIGIN_OFFSETS);
        default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
        }
        break;

      case 93: SET_MODAL(MODAL_GROUP_G5, feed_rate_mode, INVERSE_TIME_MODE);
      case 94: SET_MODAL(MODAL_GROUP_G5, feed_rate_mode, UNITS_PER_MINUTE_MODE);
        // case 95:
        // SET_MODAL(MODAL_GROUP_G5, feed_rate_mode, UNITS_PER_REVOLUTION_MODE);
      default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
      }
      break;

    case 'M':
      switch ((uint8_t)value) {
      case 0:
        SET_MODAL(MODAL_GROUP_M4, program_flow, PROGRAM_STOP);
      case 1:
        SET_MODAL(MODAL_GROUP_M4, program_flow, PROGRAM_OPTIONAL_STOP);
      case 60:
        SET_MODAL(MODAL_GROUP_M4, program_flow, PROGRAM_PALLET_CHANGE_STOP);
      case 2: case 30:
        SET_MODAL(MODAL_GROUP_M4, program_flow, PROGRAM_END);
      case 3: SET_MODAL(MODAL_GROUP_M7, spindle_mode, SPINDLE_CW);
      case 4: SET_MODAL(MODAL_GROUP_M7, spindle_mode, SPINDLE_CCW);
      case 5: SET_MODAL(MODAL_GROUP_M7, spindle_mode, SPINDLE_OFF);
      case 6: SET_NON_MODAL(tool_change, true);
      case 7: SET_MODAL(MODAL_GROUP_M8, mist_coolant, true);
      case 8: SET_MODAL(MODAL_GROUP_M8, flood_coolant, true);
      case 9: SET_MODAL(MODAL_GROUP_M8, flood_coolant, false); // Also mist
      case 48: SET_MODAL(MODAL_GROUP_M9, override_enables, true);
      case 49: SET_MODAL(MODAL_GROUP_M9, override_enables, false);
      case 50: SET_MODAL(MODAL_GROUP_M9, feed_override_enable, true);
      case 51: SET_MODAL(MODAL_GROUP_M9, spindle_override_enable, true);
      default: status = STAT_MCODE_COMMAND_UNSUPPORTED;
      }
      break;

    case 'T': SET_NON_MODAL(tool_select, (uint8_t)trunc(value));
    case 'F': SET_NON_MODAL(feed_rate, value);
      // used for dwell time, G10 coord select, rotations
    case 'P': SET_NON_MODAL(parameter, value);
    case 'S': SET_NON_MODAL(spindle_speed, value);
    case 'X': SET_NON_MODAL(target[AXIS_X], value);
    case 'Y': SET_NON_MODAL(target[AXIS_Y], value);
    case 'Z': SET_NON_MODAL(target[AXIS_Z], value);
    case 'A': SET_NON_MODAL(target[AXIS_A], value);
    case 'B': SET_NON_MODAL(target[AXIS_B], value);
    case 'C': SET_NON_MODAL(target[AXIS_C], value);
      // case 'U': SET_NON_MODAL(target[AXIS_U], value); // reserved
      // case 'V': SET_NON_MODAL(target[AXIS_V], value); // reserved
      // case 'W': SET_NON_MODAL(target[AXIS_W], value); // reserved
    case 'I': SET_NON_MODAL(arc_offset[0], value);
    case 'J': SET_NON_MODAL(arc_offset[1], value);
    case 'K': SET_NON_MODAL(arc_offset[2], value);
    case 'R': SET_NON_MODAL(arc_radius, value);
    case 'N': SET_NON_MODAL(line, (uint32_t)value); // line number
    case 'L': break; // not used for anything
    case 0: break;
    default: status = STAT_GCODE_COMMAND_UNSUPPORTED;
    }

    if (status != STAT_OK) break;
  }

  if (status != STAT_OK && status != STAT_COMPLETE) return status;
  RITORNO(_validate_gcode_block());

  return _execute_gcode_block();        // if successful execute the block
}


/// Parse a block (line) of gcode
/// Top level of gcode parser. Normalizes block and looks for special cases
stat_t gc_gcode_parser(char *block) {
  char *str = block;                    // gcode command or 0 string
  char none = 0;
  char *com = &none;                    // gcode comment or 0 string
  char *msg = &none;                    // gcode message or 0 string
  uint8_t block_delete_flag;

  _normalize_gcode_block(str, &com, &msg, &block_delete_flag);

  // Block delete omits the line if a / char is present in the first space
  // For now this is unconditional and will always delete
  if (block_delete_flag) return STAT_NOOP;

  // queue a "(MSG" response
  if (*msg) mach_message(msg);            // queue the message

  return _parse_gcode_block(block);
}
