/******************************************************************************\

                   This file is part of the TinyG firmware.

                     Copyright (c) 2016, Buildbotics LLC
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                            joseph@buildbotics.com

\******************************************************************************/

#include "command.h"

#include "gcode_parser.h"
#include "usart.h"
#include "hardware.h"
#include "report.h"
#include "vars.h"
#include "config.h"

#include <avr/pgmspace.h>

#include <stdio.h>
#include <ctype.h>


static char input[INPUT_BUFFER_LEN];


// Command forward declarations
// (Don't be afraid, X-Macros rock!)
#define CMD(name, func, minArgs, maxArgs, help) \
  uint8_t func(int, char *[]);

#include "command.def"
#undef CMD

// Command names & help
#define CMD(name, func, minArgs, maxArgs, help)      \
  static const char pstr_##name[] PROGMEM = #name;   \
  static const char name##_help[] PROGMEM = help;

#include "command.def"
#undef CMD

// Command table
#define CMD(name, func, minArgs, maxArgs, help) \
  {pstr_##name, func, minArgs, maxArgs, name##_help},

static const command_t commands[] PROGMEM = {
#include "command.def"
#undef CMD
  {}, // Sentinel
};


stat_t command_dispatch() {
  // Read input line or return if incomplete, usart_gets() is non-blocking
  ritorno(usart_gets(input, sizeof(input)));

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

  int i = command_find(argv[0]);
  if (i != -1) {
    putchar('\n');

    uint8_t minArgs = pgm_read_byte(&commands[i].minArgs);
    uint8_t maxArgs = pgm_read_byte(&commands[i].maxArgs);

    if (argc <= minArgs) printf_P(PSTR("Too few arguments\n"));
    else if (maxArgs < argc - 1) printf_P(PSTR("Too many arguments\n"));
    else {
      command_cb_t cb = pgm_read_word(&commands[i].cb);
      status = cb(argc, argv);
    }

  } else printf_P(PSTR("Unknown command '%s'\n"), argv[0]);

  return status;
}


int command_eval(char *cmd) {
  while (*cmd && isspace(*cmd)) cmd++;

  if (!*cmd) {
    report_request_full();
    return STAT_OK;
  }

  if (*cmd == '{') return vars_parser(cmd);
  if (*cmd != '$') return gc_gcode_parser(cmd);

  // Parse line
  char *p = cmd + 1;
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


// Command functions
void static print_command_help(int i) {
  static const char fmt[] PROGMEM = "  %-8S  %S\n";
  const char *name = pgm_read_word(&commands[i].name);
  const char *help = pgm_read_word(&commands[i].help);

  printf_P(fmt, name, help);
}


uint8_t command_help(int argc, char *argv[]) {
  if (argc == 2) {
    int i = command_find(argv[0]);

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
  }

  puts_P(PSTR("\nVariables:"));
  vars_print_help();

  return 0;
}


uint8_t command_reboot(int argc, char *argv[]) {
  hw_request_hard_reset();
  return 0;
}
