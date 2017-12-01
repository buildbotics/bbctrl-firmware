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

#pragma once

#include "config.h"
#include "action.h"

#include <stdint.h>
#include <stdbool.h>


// Called by state.c
void queue_flush();
bool queue_is_empty();
int queue_get_room();
int queue_get_fill();

void queue_push(action_t action);
void queue_push_float(action_t action, float value);
void queue_push_int(action_t action, int32_t value);
void queue_push_bool(action_t action, bool value);
void queue_push_pair(action_t action, int16_t left, int16_t right);

void queue_pop();

action_t queue_head();
float queue_head_float();
int32_t queue_head_int();
bool queue_head_bool();
int16_t queue_head_left();
int16_t queue_head_right();
