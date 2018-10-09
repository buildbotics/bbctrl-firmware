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

#include <stdint.h>
#include <stdbool.h>


typedef enum {
  OUT_LO,
  OUT_HI,
  OUT_TRI,
} output_state_t;


/// OUT_<inactive>_<active>
typedef enum {
  OUT_DISABLED,
  OUT_LO_HI,
  OUT_HI_LO,
  OUT_TRI_LO,
  OUT_TRI_HI,
  OUT_LO_TRI,
  OUT_HI_TRI,
} output_mode_t;


void outputs_init();
bool outputs_is_active(uint8_t pin);
void outputs_set_active(uint8_t pin, bool active);
bool outputs_toggle(uint8_t pin);
void outputs_set_mode(uint8_t pin, output_mode_t mode);
output_state_t outputs_get_state(uint8_t pin);
void outputs_stop();
