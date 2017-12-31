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

#include "type.h"
#include "base64.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>


#define TYPEDEF(TYPE, DEF)                                              \
  static const char TYPE##_name [] PROGMEM = "<" #TYPE ">";             \
  pstr type_get_##TYPE##_name_pgm() {return TYPE##_name;}

#include "type.def"
#undef TYPEDEF


// String
bool type_eq_str(str a, str b) {return a == b;}
void type_print_str(str s) {printf_P(PSTR("\"%s\""), s);}
float type_str_to_float(str s) {return 0;}
str type_parse_str(const char *s) {return s;}

// Program string
bool type_eq_pstr(pstr a, pstr b) {return a == b;}
void type_print_pstr(pstr s) {printf_P(PSTR("\"%"PRPSTR"\""), s);}
const char *type_parse_pstr(const char *value) {return value;}
float type_pstr_to_float(pstr s) {return 0;}


// Flags
bool type_eq_flags(flags a, flags b) {return a == b;}
float type_flags_to_float(flags x) {return x;}


void type_print_flags(flags x) {
  extern void print_status_flags(flags x);
  print_status_flags(x);
}

flags type_parse_flags(const char *s) {return 0;} // Not used


// Float
bool type_eq_f32(float a, float b) {return a == b || (isnan(a) && isnan(b));}
float type_f32_to_float(float x) {return x;}


void type_print_f32(float x) {
  if (isnan(x)) printf_P(PSTR("\"nan\""));
  else if (isinf(x)) printf_P(PSTR("\"%cinf\""), x < 0 ? '-' : '+');

  else {
    char buf[20];

    int len = sprintf_P(buf, PSTR("%.3f"), x);

    // Remove trailing zeros
    for (int i = len; 0 < i; i--) {
      if (buf[i - 1] == '.') buf[i - 1] = 0;
      else if (buf[i - 1] == '0') {
        buf[i - 1] = 0;
        continue;
      }

      break;
    }

    printf("%s", buf);
  }
}


float type_parse_f32(const char *value) {
  while (*value && isspace(*value)) value++;

  if (*value == ':') {
    value++;
    if (strnlen(value, 6) != 6) return NAN;

    float f;
    return b64_decode_float(value, &f) ? f : NAN;
  }

  return strtod(value, 0);
}


// bool
bool type_eq_bool(bool a, bool b) {return a == b;}
float type_bool_to_float(bool x) {return x;}
void type_print_bool(bool x) {printf_P(x ? PSTR("true") : PSTR("false"));}


bool type_parse_bool(const char *value) {
  return !strcasecmp(value, "true") || type_parse_f32(value);
}


// s8
bool type_eq_s8(s8 a, s8 b) {return a == b;}
float type_s8_to_float(s8 x) {return x;}
void type_print_s8(s8 x) {printf_P(PSTR("%"PRIi8), x);}
s8 type_parse_s8(const char *value) {return strtol(value, 0, 0);}


// u8
bool type_eq_u8(u8 a, u8 b) {return a == b;}
float type_u8_to_float(u8 x) {return x;}
void type_print_u8(u8 x) {printf_P(PSTR("%"PRIu8), x);}
u8 type_parse_u8(const char *value) {return strtol(value, 0, 0);}


// u16
bool type_eq_u16(u16 a, u16 b) {return a == b;}
float type_u16_to_float(u16 x) {return x;}
void type_print_u16(u16 x) {printf_P(PSTR("%"PRIu16), x);}
u16 type_parse_u16(const char *value) {return strtoul(value, 0, 0);}


// s32
bool type_eq_s32(s32 a, s32 b) {return a == b;}
float type_s32_to_float(s32 x) {return x;}
void type_print_s32(s32 x) {printf_P(PSTR("%"PRIi32), x);}
s32 type_parse_s32(const char *value) {return strtol(value, 0, 0);}


// u32
bool type_eq_u32(u32 a, u32 b) {return a == b;}
float type_u32_to_float(u32 x) {return x;}
void type_print_u32(u32 x) {printf_P(PSTR("%"PRIu32), x);}
u32 type_parse_u32(const char *value) {return strtol(value, 0, 0);}


type_u type_parse(type_t type, const char *s) {
  type_u value;

  switch (type) {
#define TYPEDEF(TYPE, ...)                                              \
    case TYPE_##TYPE: value._##TYPE = type_parse_##TYPE(s); break;
#include "type.def"
#undef TYPEDEF
  }

  return value;
}


void type_print(type_t type, type_u value) {
  switch (type) {
#define TYPEDEF(TYPE, ...)                                      \
    case TYPE_##TYPE: type_print_##TYPE(value._##TYPE); break;
#include "type.def"
#undef TYPEDEF
  }
}


float type_to_float(type_t type, type_u value) {
  switch (type) {
#define TYPEDEF(TYPE, ...)                                          \
    case TYPE_##TYPE: return type_##TYPE##_to_float(value._##TYPE);
#include "type.def"
#undef TYPEDEF
  }
  return 0;
}
