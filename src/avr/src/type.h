/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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

#include "pgmspace.h"

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
#define TYPEDEF(TYPE, DEF)                           \
  pstr type_get_##TYPE##_name_pgm();                 \
  bool type_eq_##TYPE(TYPE a, TYPE b);               \
  TYPE type_parse_##TYPE(const char *s);             \
  void type_print_##TYPE(TYPE x);                    \
  float type_##TYPE##_to_float(TYPE x);
#include "type.def"
#undef TYPEDEF


type_u type_parse(type_t type, const char *s);
void type_print(type_t type, type_u value);
float type_to_float(type_t type, type_u value);
