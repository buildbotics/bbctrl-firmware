/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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

#pragma once

#include "pgmspace.h"
#include "status.h"

#include <stdint.h>
#include <stdbool.h>


// Define types
#define TYPEDEF(TYPE, DEF) typedef DEF TYPE;
#include "type.def"
#undef TYPEDEF

typedef enum {
#define TYPEDEF(TYPE, ...) TYPE_##TYPE,
#include "type.def"
#undef TYPEDEF
} type_t;


typedef union {
#define TYPEDEF(TYPE, ...) TYPE _##TYPE;
#include "type.def"
#undef TYPEDEF
} type_u;


// Define functions
#define TYPEDEF(TYPE, DEF)                                  \
  pstr type_get_##TYPE##_name_pgm();                        \
  bool type_eq_##TYPE(TYPE a, TYPE b);                      \
  TYPE type_parse_##TYPE(const char *s, stat_t *status);    \
  void type_print_##TYPE(TYPE x);
#include "type.def"
#undef TYPEDEF


type_u type_parse(type_t type, const char *s, stat_t *status);
void type_print(type_t type, type_u value);
