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

#include "vars.h"

#include "cpp_magic.h"
#include "status.h"
#include "hardware.h"
#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include <avr/pgmspace.h>


typedef uint8_t flags_t;
typedef const char *string;
typedef PGM_P pstring;


// Format strings
static const char code_fmt[] PROGMEM = "\"%s\":";
static const char indexed_code_fmt[] PROGMEM = "\"%c%s\":";


// Type names
static const char bool_name [] PROGMEM = "<bool>";
#define TYPE_NAME(TYPE) static const char TYPE##_name [] PROGMEM = "<" #TYPE ">"
MAP(TYPE_NAME, SEMI, flags_t, string, pstring, float, uint8_t, uint16_t,
    int32_t, char);


// String
static void var_print_string(string s) {
  printf_P(PSTR("\"%s\""), s);
}

// Program string
static void var_print_pstring(pstring s) {
  printf_P(PSTR("\"%S\""), s);
}


// Flags
extern void print_status_flags(uint8_t x);

static void var_print_flags_t(uint8_t x) {
  print_status_flags(x);
}


// Float
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


static float var_parse_float(const char *value) {
  return strtod(value, 0);
}


// Bool
static void var_print_bool(bool x) {
  printf_P(x ? PSTR("true") : PSTR("false"));
}


static bool var_parse_bool(const char *value) {
  return !strcasecmp(value, "true") || var_parse_float(value);
}


// Char
static void var_print_char(char x) {putchar('"'); putchar(x); putchar('"');}
static char var_parse_char(const char *value) {return value[0];}


// int8
#if 0
static void var_print_int8_t(int8_t x) {
  printf_P(PSTR("%"PRIi8), x);
}


static int8_t var_parse_int8_t(const char *value) {
  return strtol(value, 0, 0);
}
#endif

// uint8
static void var_print_uint8_t(uint8_t x) {
  printf_P(PSTR("%"PRIu8), x);
}


static uint8_t var_parse_uint8_t(const char *value) {
  return strtol(value, 0, 0);
}


// unit16
static void var_print_uint16_t(uint16_t x) {
  printf_P(PSTR("%"PRIu16), x);
}


static uint16_t var_parse_uint16_t(const char *value) {
  return strtoul(value, 0, 0);
}


// int32
static void var_print_int32_t(uint32_t x) {
  printf_P(PSTR("%"PRIi32), x);
}


// Ensure no code is used more than once
enum {
#define VAR(NAME, CODE, ...) var_code_##CODE##_reuse,
#include "vars.def"
#undef VAR
};

// Var forward declarations
#define VAR(NAME, CODE, TYPE, INDEX, SET, ...)          \
  TYPE get_##NAME(IF(INDEX)(int index));                \
  IF(SET)                                               \
  (void set_##NAME(IF(INDEX)(int index,) TYPE value);)

#include "vars.def"
#undef VAR

// Var names, count & help
#define VAR(NAME, CODE, TYPE, INDEX, SET, HELP)             \
  static const char NAME##_name[] PROGMEM = #NAME;          \
  static const char NAME##_help[] PROGMEM = HELP;

#include "vars.def"
#undef VAR

// Last value
#define VAR(NAME, CODE, TYPE, INDEX, ...)       \
  static TYPE NAME##_state IF(INDEX)([INDEX]);

#include "vars.def"
#undef VAR



void vars_init() {
  // Initialize var state
#define VAR(NAME, CODE, TYPE, INDEX, ...)                       \
  IF(INDEX)(for (int i = 0; i < INDEX; i++))                    \
    (NAME##_state)IF(INDEX)([i]) = get_##NAME(IF(INDEX)(i));

#include "vars.def"
#undef VAR
}


void vars_report(bool full) {
  // Save and disable watchdog
  uint8_t wd_state = hw_disable_watchdog();

  bool reported = false;

#define VAR(NAME, CODE, TYPE, INDEX, ...)                       \
  IF(INDEX)(for (int i = 0; i < (INDEX ? INDEX : 1); i++)) {    \
    TYPE value = get_##NAME(IF(INDEX)(i));                      \
    TYPE last = (NAME##_state)IF(INDEX)([i]);                   \
                                                                \
    if (value != last || full) {                                \
      (NAME##_state)IF(INDEX)([i]) = value;                     \
                                                                \
      if (!reported) {                                          \
        reported = true;                                        \
        putchar('{');                                           \
      } else putchar(',');                                      \
                                                                \
      printf_P                                                  \
        (IF_ELSE(INDEX)(indexed_code_fmt, code_fmt),            \
         IF(INDEX)(INDEX##_LABEL[i],) #CODE);                   \
                                                                \
      var_print_##TYPE(value);                                  \
    }                                                           \
  }

#include "vars.def"
#undef VAR

  if (reported) printf("}\n");

  // Restore watchdog
  hw_restore_watchdog(wd_state);
}


int vars_find(const char *name) {
  uint8_t i = 0;
  uint8_t n = 0;
  unsigned len = strlen(name);

  if (!len) return -1;

#define VAR(NAME, CODE, TYPE, INDEX, ...)                               \
  if (!strcmp(IF_ELSE(INDEX)(name + 1, name), #CODE)) {                 \
    IF(INDEX)                                                           \
      (i = strchr(INDEX##_LABEL, name[0]) - INDEX##_LABEL;              \
       if (INDEX <= i) return -1);                                      \
    return n;                                                           \
  }                                                                     \
  n++;

#include "vars.def"
#undef VAR

  return -1;
}


bool vars_print(const char *name) {
  uint8_t i;
  unsigned len = strlen(name);

  if (!len) return false;

#define VAR(NAME, CODE, TYPE, INDEX, ...)                               \
  if (!strcmp(IF_ELSE(INDEX)(name + 1, name), #CODE)) {                 \
    IF(INDEX)                                                           \
      (i = strchr(INDEX##_LABEL, name[0]) - INDEX##_LABEL;              \
       if (INDEX <= i) return false);                                   \
                                                                        \
    putchar('{');                                                       \
    printf_P                                                            \
      (IF_ELSE(INDEX)(indexed_code_fmt, code_fmt),                      \
       IF(INDEX)(INDEX##_LABEL[i],) #CODE);                             \
    var_print_##TYPE(get_##NAME(IF(INDEX)(i)));                         \
    putchar('}');                                                       \
                                                                        \
    return true;                                                        \
  }

#include "vars.def"
#undef VAR

  return false;
}


bool vars_set(const char *name, const char *value) {
  uint8_t i;
  unsigned len = strlen(name);

  if (!len) return false;

#define VAR(NAME, CODE, TYPE, INDEX, SET, ...)                          \
  IF(SET)                                                               \
    (if (!strcmp(IF_ELSE(INDEX)(name + 1, name), #CODE)) {              \
      IF(INDEX)                                                         \
        (i = strchr(INDEX##_LABEL, name[0]) - INDEX##_LABEL;            \
         if (INDEX <= i) return false);                                 \
                                                                        \
      TYPE x = var_parse_##TYPE(value);                                 \
      set_##NAME(IF(INDEX)(i,) x);                                      \
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
  static const char fmt[] PROGMEM = "  $%-5s %-20S %-16S  %S\n";

  // Save and disable watchdog
  uint8_t wd_state = hw_disable_watchdog();

#define VAR(NAME, CODE, TYPE, ...)                               \
  printf_P(fmt, #CODE, NAME##_name, TYPE##_name, NAME##_help);
#include "vars.def"
#undef VAR

  // Restore watchdog
  hw_restore_watchdog(wd_state);
}
