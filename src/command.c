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

#include "command.h"

#include "gcode_parser.h"
#include "usart.h"
#include "hardware.h"
#include "report.h"
#include "vars.h"
#include "estop.h"
#include "homing.h"
#include "probing.h"
#include "i2c.h"
#include "plan/jog.h"
#include "plan/calibrate.h"
#include "plan/buffer.h"
#include "plan/arc.h"
#include "plan/state.h"
#include "config.h"

#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


static char *_cmd = 0;


static void _reboot()     {hw_request_hard_reset();}


static unsigned _parse_axis(uint8_t axis) {
  switch (axis) {
  case 'x': return 0; case 'y': return 1; case 'z': return 2;
  case 'a': return 3; case 'b': return 4; case 'c': return 5;
  case 'X': return 0; case 'Y': return 1; case 'Z': return 2;
  case 'A': return 3; case 'B': return 4; case 'C': return 5;
  default: return axis;
  }
}


static void command_i2c_cb(i2c_cmd_t cmd, uint8_t *data, uint8_t length) {
  switch (cmd) {
  case I2C_NULL:                                           break;
  case I2C_ESTOP:          estop_trigger(STAT_ESTOP_USER); break;
  case I2C_CLEAR:          estop_clear();                  break;
  case I2C_PAUSE:          mp_request_hold();              break;
  case I2C_OPTIONAL_PAUSE: mp_request_optional_pause();    break;
  case I2C_RUN:            mp_request_start();             break;
  case I2C_STEP:           mp_request_step();              break;
  case I2C_FLUSH:          mp_request_flush();             break;
  case I2C_REPORT:         report_request_full();          break;
  case I2C_HOME:                                           break; // TODO
  case I2C_REBOOT:         _reboot();                      break;
  case I2C_ZERO:
    if (length == 0) mach_zero_all();
    else if (length == 1) mach_zero_axis(_parse_axis(*data));
    break;
  }
}


void command_init() {
  i2c_set_read_callback(command_i2c_cb);
}


// Command forward declarations
// (Don't be afraid, X-Macros rock!)
#define CMD(NAME, ...)                          \
  uint8_t command_##NAME(int, char *[]);

#include "command.def"
#undef CMD

// Command names & help
#define CMD(NAME, MINARGS, MAXARGS, HELP)      \
  static const char pstr_##NAME[] PROGMEM = #NAME;   \
  static const char NAME##_help[] PROGMEM = HELP;

#include "command.def"
#undef CMD

// Command table
#define CMD(NAME, MINARGS, MAXARGS, HELP)                       \
  {pstr_##NAME, command_##NAME, MINARGS, MAXARGS, NAME##_help},

static const command_t commands[] PROGMEM = {
#include "command.def"
#undef CMD
  {}, // Sentinel
};


int command_find(const char *match) {
  for (int i = 0; ; i++) {
    const char *name = pgm_read_word(&commands[i].name);
    if (!name) break;

    if (strcmp_P(match, name) == 0) return i;
  }

  return -1;
}


int command_exec(int argc, char *argv[]) {
  putchar('\n');

  int i = command_find(argv[0]);
  if (i != -1) {
    uint8_t min_args = pgm_read_byte(&commands[i].min_args);
    uint8_t max_args = pgm_read_byte(&commands[i].max_args);

    if (argc <= min_args) return STAT_TOO_FEW_ARGUMENTS;
    else if (max_args < argc - 1) return STAT_TOO_MANY_ARGUMENTS;
    else {
      command_cb_t cb = pgm_read_word(&commands[i].cb);
      return cb(argc, argv);
    }

  } else if (argc != 1)
    return STAT_INVALID_OR_MALFORMED_COMMAND;

  // Get or set variable
  char *value = strchr(argv[0], '=');
  if (value) {
    *value++ = 0;
    if (vars_set(argv[0], value)) return STAT_OK;

  } else if (vars_print(argv[0])) {
    putchar('\n');
    return STAT_OK;
  }

  return STAT_UNRECOGNIZED_NAME;
}


int command_parser(char *cmd) {
  // Parse line
  char *p = cmd + 1; // Skip `$`
  int argc = 0;
  char *argv[MAX_ARGS] = {0};

  if (cmd[1] == '$' && !cmd[2]) {
    report_request_full(); // Full report
    return STAT_OK;
  }

  while (argc < MAX_ARGS && *p) {
    // Skip space
    while (*p && isspace(*p)) *p++ = 0;

    // Start of token
    if (*p) argv[argc++] = p;

    // Find end
    while (*p && !isspace(*p)) p++;
  }

  // Exec command
  if (argc) return command_exec(argc, argv);

  return STAT_OK;
}


static char *_command_next() {
  if (_cmd) return _cmd;

  // Get next command
  _cmd = usart_readline();
  if (!_cmd) return 0;

  // Remove leading whitespace
  while (*_cmd && isspace(*_cmd)) _cmd++;

  // Remove trailing whitespace
  for (size_t len = strlen(_cmd); len && isspace(_cmd[len - 1]); len--)
    _cmd[len - 1] = 0;

  return _cmd;
}


void command_callback() {
  if (!_command_next()) return;

  stat_t status = STAT_OK;

  switch (*_cmd) {
  case 0: break; // Empty line
  case '{': status = vars_parser(_cmd); break;
  case '$': status = command_parser(_cmd); break;
  case '%': break; // GCode program separator, ignore it

  default:
    if (estop_triggered()) {status = STAT_MACHINE_ALARMED; break;}
    else if (mp_is_flushing()) break; // Flush GCode command
    else if (!mp_queue_get_room() ||
             mp_is_resuming() ||
             mach_arc_active() ||
             mach_is_homing() ||
             mach_is_probing() ||
             calibrate_busy() ||
             mp_jog_busy()) return; // Wait

    // Parse and execute GCode command
    status = gc_gcode_parser(_cmd);
  }

  _cmd = 0; // Command consumed
  report_request();

  if (status) status_error(status);
}


// Command functions
void static print_command_help(int i) {
  static const char fmt[] PROGMEM = "  $%-12S  %S\n";
  const char *name = pgm_read_word(&commands[i].name);
  const char *help = pgm_read_word(&commands[i].help);

  printf_P(fmt, name, help);
}


uint8_t command_help(int argc, char *argv[]) {
  if (argc == 2) {
    int i = command_find(argv[1]);

    if (i == -1) return STAT_UNRECOGNIZED_NAME;
    else print_command_help(i);

    return STAT_OK;
  }

  puts_P(PSTR("\nLine editing:\n"
              "  ENTER     Submit current command line.\n"
              "  BS        Backspace, delete last character.\n"
              "  CTRL-X    Cancel current line entry."));

  puts_P(PSTR("\nCommands:"));
  for (int i = 0; ; i++) {
    const char *name = pgm_read_word(&commands[i].name);
    if (!name) break;
    print_command_help(i);
    wdt_reset();
  }

  puts_P(PSTR("\nVariables:"));
  vars_print_help();

  return STAT_OK;
}


uint8_t command_reboot(int argc, char *argv[]) {
  _reboot();
  return 0;
}


uint8_t command_bootloader(int argc, char *argv[]) {
  hw_request_bootloader();
  return 0;
}


uint8_t command_save(int argc, char *argv[]) {
  vars_save();
  return 0;
}


uint8_t command_valid(int argc, char *argv[]) {
  printf_P(vars_valid() ? PSTR("true\n") : PSTR("false\n"));
  return 0;
}


uint8_t command_restore(int argc, char *argv[]) {
  return vars_restore();
}


uint8_t command_clear(int argc, char *argv[]) {
  vars_clear();
  return 0;
}


uint8_t command_messages(int argc, char *argv[]) {
  status_help();
  return 0;
}


uint8_t command_resume(int argc, char *argv[]) {
  mp_request_resume();
  return 0;
}
