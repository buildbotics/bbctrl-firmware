/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/

#include "type.h"
#include "base64.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <inttypes.h>


#define TYPEDEF(TYPE, DEF)                                              \
  static const char TYPE##_name [] PROGMEM = "<" #TYPE ">";             \
  pstr type_get_##TYPE##_name_pgm() {return TYPE##_name;}

#include "type.def"
#undef TYPEDEF


// String
bool type_eq_str(str a, str b) {return a == b;}
void type_print_str(str s) {printf_P(PSTR("\"%s\""), s);}
str type_parse_str(const char *s, stat_t *) {return s;}

// Program string
bool type_eq_pstr(pstr a, pstr b) {return a == b;}
void type_print_pstr(pstr s) {printf_P(PSTR("\"%" PRPSTR "\""), s);}
const char *type_parse_pstr(const char *value, stat_t *) {return value;}


// Float
bool type_eq_f32(float a, float b) {return a == b || (isnan(a) && isnan(b));}


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

    printf(buf);
  }
}


float type_parse_f32(const char *value, stat_t *status) {
  while (*value && isspace(*value)) value++;

  if (*value == ':') {
    value++;
    if (strnlen(value, 6) != 6) {
      if (status) *status = STAT_INVALID_VALUE;
      return NAN;
    }

    float f;
    if (b64_decode_float(value, &f)) return f;
    else {
      if (status) *status = STAT_BAD_FLOAT;
      return NAN;
    }
  }

  char *endptr = 0;
  float x = strtod(value, &endptr);

  if (endptr == value) {
    if (status) *status = STAT_BAD_FLOAT;
    return NAN;
  }

  return x;
}


// bool
bool type_eq_b8(bool a, bool b) {return a == b;}
void type_print_b8(bool x) {printf_P(x ? PSTR("true") : PSTR("false"));}


bool type_parse_b8(const char *value, stat_t *status) {
  return !strcasecmp(value, "true") || type_parse_f32(value, status);
}


// s8
bool type_eq_s8(s8 a, s8 b) {return a == b;}
void type_print_s8(s8 x) {printf_P(PSTR("%" PRIi8), x);}


s8 type_parse_s8(const char *value, stat_t *status) {
  char *endptr = 0;
  s8 x = strtol(value, &endptr, 0);
  if (endptr == value && status) *status = STAT_BAD_INT;
  return x;
}


// u8
bool type_eq_u8(u8 a, u8 b) {return a == b;}
void type_print_u8(u8 x) {printf_P(PSTR("%" PRIu8), x);}


u8 type_parse_u8(const char *value, stat_t *status) {
  char *endptr = 0;
  u8 x = strtoul(value, &endptr, 0);
  if (endptr == value && status) *status = STAT_BAD_INT;
  return x;
}


// u16
bool type_eq_u16(u16 a, u16 b) {return a == b;}
void type_print_u16(u16 x) {printf_P(PSTR("%" PRIu16), x);}


u16 type_parse_u16(const char *value, stat_t *status) {
  char *endptr = 0;
  u16 x = strtoul(value, &endptr, 0);
  if (endptr == value && status) *status = STAT_BAD_INT;
  return x;
}


// s32
bool type_eq_s32(s32 a, s32 b) {return a == b;}
void type_print_s32(s32 x) {printf_P(PSTR("%" PRIi32), x);}


s32 type_parse_s32(const char *value, stat_t *status) {
  char *endptr = 0;
  s32 x = strtol(value, &endptr, 0);
  if (endptr == value && status) *status = STAT_BAD_INT;
  return x;
}


// u32
bool type_eq_u32(u32 a, u32 b) {return a == b;}
void type_print_u32(u32 x) {printf_P(PSTR("%" PRIu32), x);}


u32 type_parse_u32(const char *value, stat_t *status) {
  char *endptr = 0;
  u32 x = strtoul(value, &endptr, 0);
  if (endptr == value && status) *status = STAT_BAD_INT;
  return x;
}


type_u type_parse(type_t type, const char *s, stat_t *status) {
  type_u value;

  if (status) *status = STAT_OK;

  switch (type) {
#define TYPEDEF(TYPE, ...)                                              \
    case TYPE_##TYPE: value._##TYPE = type_parse_##TYPE(s, status); break;
#include "type.def"
#undef TYPEDEF
    default: if (status) *status = STAT_INVALID_TYPE;
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
