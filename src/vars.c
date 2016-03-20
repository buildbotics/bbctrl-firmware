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

#include "vars.h"

#include "cpp_magic.h"
#include "status.h"
#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <avr/pgmspace.h>
#include <avr/wdt.h>

// Type names
static const char bool_name [] PROGMEM = "<bool>";
#define TYPE_NAME(TYPE) static const char TYPE##_name [] PROGMEM = "<" #TYPE ">"
MAP(TYPE_NAME, SEMI, string, float, int8_t, uint8_t, uint16_t);


static void var_print_float(float x) {
  char buf[20];

  int len = sprintf_P(buf, PSTR("%.6f"), x);

  // Remove trailing zeros
  for (int i = len; 0 < i; i--) {
    if (buf[i - 1] == '.') buf[i - 1] = 0;
    else if (buf[i - 1] == '0') {
      buf[i - 1] = 0;
      continue;
    }

    break;
  }

  printf(buf);
}


static void var_print_int8_t(int8_t x) {
  printf_P(PSTR("%"PRIi8), x);
}


static void var_print_uint8_t(uint8_t x) {
  printf_P(PSTR("%"PRIu8), x);
}


static void var_print_uint16_t(uint16_t x) {
  printf_P(PSTR("%"PRIu16), x);
}


static float var_parse_float(const char *value) {
  return strtod(value, 0);
}


static int8_t var_parse_int8_t(const char *value) {
  return strtol(value, 0, 0);
}

static uint8_t var_parse_uint8_t(const char *value) {
  return strtol(value, 0, 0);
}


static uint16_t var_parse_uint16_t(const char *value) {
  return strtol(value, 0, 0);
}


// Var forward declarations
#define VAR(NAME, CODE, TYPE, INDEX, SET, ...)         \
  TYPE get_##NAME(IF(INDEX)(int index));               \
  IF(SET)                                              \
  (void set_##NAME(IF(INDEX)(int index,) TYPE value);)

#include "vars.def"
#undef VAR

// Var names, count & help
#define VAR(NAME, CODE, TYPE, INDEX, SET, DEFAULT, HELP)    \
  static const char NAME##_name[] PROGMEM = #NAME;          \
  static const int NAME##_count = INDEX ? INDEX : 1;        \
  static const char NAME##_help[] PROGMEM = HELP;

#include "vars.def"
#undef VAR

// Last value
#define VAR(NAME, CODE, TYPE, INDEX, ...)       \
  static TYPE NAME##_last IF(INDEX)([INDEX]);

#include "vars.def"
#undef VAR


void vars_init() {
#define VAR(NAME, CODE, TYPE, INDEX, SET, DEFAULT, ...)  \
  IF(SET)                                         \
    (IF(INDEX)(for (int i = 0; i < INDEX; i++)) { \
      set_##NAME(IF(INDEX)(i,) DEFAULT);          \
      (NAME##_last)IF(INDEX)([i]) = DEFAULT;      \
    })

#include "vars.def"
#undef VAR
}


void vars_report(bool full) {
  bool reported = false;

  static const char value_fmt[] PROGMEM = "\"%s\":";
  static const char index_value_fmt[] PROGMEM = "\"%c%s\":";

#define VAR(NAME, CODE, TYPE, INDEX, ...)                \
  IF(INDEX)(for (int i = 0; i < NAME##_count; i++)) {   \
    TYPE value = get_##NAME(IF(INDEX)(i));              \
                                                        \
    if (value != (NAME##_last)IF(INDEX)([i]) || full) { \
      (NAME##_last)IF(INDEX)([i]) = value;              \
                                                        \
      if (!reported) {                                  \
        reported = true;                                \
        putchar('{');                                   \
      } else putchar(',');                              \
                                                        \
      printf_P                                          \
        (IF_ELSE(INDEX)(index_value_fmt, value_fmt),    \
         IF(INDEX)(INDEX##_LABEL[i],) CODE);            \
                                                        \
      var_print_##TYPE(value);                          \
    }                                                   \
    wdt_reset();                                        \
  }

#include "vars.def"
#undef VAR

  if (reported) printf("}\n");
}


bool vars_set(const char *name, const char *value) {
  uint8_t i;
  unsigned len = strlen(name);


  if (!len) return false;

#define VAR(NAME, CODE, TYPE, INDEX, SET, ...)                          \
  IF(SET)                                                               \
    (if (!strcmp(IF_ELSE(INDEX)(name + 1, name), CODE)) {               \
      IF(INDEX)                                                         \
        (i = strchr(INDEX##_LABEL, name[0]) - INDEX##_LABEL;            \
         if (INDEX <= i) return false);                                 \
                                                                        \
      set_##NAME(IF(INDEX)(i ,) var_parse_##TYPE(value));               \
                                                                        \
      return true;                                                      \
    })                                                                  \

#include "vars.def"
#undef VAR

  return false;
}


int vars_parser(char *vars) {
  if (!*vars || !*vars++ == '{') return STAT_OK;

  while (*vars) {
    while (isspace(*vars)) vars++;
    if (*vars == '}') return STAT_OK;
    if (*vars != '"') return STAT_COMMAND_NOT_ACCEPTED;

    // Parse name
    vars++; // Skip "
    const char *name = vars;
    while (*vars && *vars != '"') vars++;
    *vars++ = 0;

    while (isspace(*vars)) vars++;
    if (*vars != ':') return STAT_COMMAND_NOT_ACCEPTED;
    vars++;
    while (isspace(*vars)) vars++;

    // Parse value
    const char *value = vars;
    while (*vars && *vars != ',' && *vars != '}') vars++;
    if (*vars) {
      char c = *vars;
      *vars++ = 0;
      vars_set(name, value);
      if (c == '}') break;
    }
  }

  return STAT_OK;
}


void vars_print_help() {
  static const char fmt[] PROGMEM = "  $%-5s %-14S %-12S  %S\n";

#define VAR(NAME, CODE, TYPE, ...)                      \
  printf_P(fmt, CODE, NAME##_name, TYPE##_name, NAME##_help);    \
  wdt_reset();
#include "vars.def"
#undef VAR
}
