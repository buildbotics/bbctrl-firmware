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
#include "plan/jog.h"
#include "plan/calibrate.h"
#include "config.h"

#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>


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


stat_t command_dispatch() {
  // Read input line or return if incomplete, usart_gets() is non-blocking
  char *input = usart_readline();
  if (!input) return STAT_EAGAIN;

  return command_eval(input);
}


int command_find(const char *match) {
  for (int i = 0; ; i++) {
    const char *name = pgm_read_word(&commands[i].name);
    if (!name) break;

    if (strcmp_P(match, name) == 0) return i;
  }

  return -1;
}


int command_exec(int argc, char *argv[]) {
  stat_t status = STAT_INVALID_OR_MALFORMED_COMMAND;

  putchar('\n');

  int i = command_find(argv[0]);
  if (i != -1) {
    uint8_t minArgs = pgm_read_byte(&commands[i].minArgs);
    uint8_t maxArgs = pgm_read_byte(&commands[i].maxArgs);

    if (argc <= minArgs) {
      printf_P(PSTR("Too few arguments\n"));
      return status;

    } else if (maxArgs < argc - 1) {
      printf_P(PSTR("Too many arguments\n"));
      return status;

    } else {
      command_cb_t cb = pgm_read_word(&commands[i].cb);
      return cb(argc, argv);
    }

  } else if (argc != 1) {
    printf_P(PSTR("Unknown command '%s' or invalid arguments\n"), argv[0]);
    return status;
  }

  // Get or set variable
  char *value = strchr(argv[0], '=');
  if (value) {
    *value++ = 0;
    if (vars_set(argv[0], value)) return STAT_OK;

  } else if (vars_print(argv[0])) {
    putchar('\n');
    return STAT_OK;
  }

  printf_P(PSTR("Unknown command or variable '%s'\n"), argv[0]);

  return status;
}


int command_parser(char *cmd) {
  // Parse line
  char *p = cmd + 1; // Skip `$`
  int argc = 0;
  char *argv[MAX_ARGS] = {};

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


int command_eval(char *cmd) {
  // Skip leading whitespace
  while (*cmd && isspace(*cmd)) cmd++;

  switch (*cmd) {
  case 0: report_request_full(); return STAT_OK;
  case '{': return vars_parser(cmd);
  case '$': return command_parser(cmd);
  default:
    if (calibrate_busy()) return STAT_OK;
    if (mp_jog_busy()) return STAT_OK;
    return gc_gcode_parser(cmd);
  }
}


// Command functions
void static print_command_help(int i) {
  static const char fmt[] PROGMEM = "  $%-8S  %S\n";
  const char *name = pgm_read_word(&commands[i].name);
  const char *help = pgm_read_word(&commands[i].help);

  printf_P(fmt, name, help);
}


uint8_t command_help(int argc, char *argv[]) {
  if (argc == 2) {
    int i = command_find(argv[1]);

    if (i == -1) {
      printf_P(PSTR("Command not found\n"));
      return STAT_INVALID_OR_MALFORMED_COMMAND;

    } else print_command_help(i);

    return 0;
  }

  puts_P(PSTR("\nCommands:"));
  for (int i = 0; ; i++) {
    const char *name = pgm_read_word(&commands[i].name);
    if (!name) break;
    print_command_help(i);
    wdt_reset();
  }

  puts_P(PSTR("\nVariables:"));
  vars_print_help();

  return 0;
}


uint8_t command_reboot(int argc, char *argv[]) {
  hw_request_hard_reset();
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
