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

#include "status.h"

#include <stdbool.h>


bool mp_runtime_is_busy();
void mp_runtime_set_busy(bool busy);

int32_t mp_runtime_get_line();
void mp_runtime_set_line(int32_t line);

uint8_t mp_runtime_get_tool();
void mp_runtime_set_tool(uint8_t tool);

float mp_runtime_get_velocity();
void mp_runtime_set_velocity(float velocity);

void mp_runtime_set_steps_from_position();

void mp_runtime_set_axis_position(uint8_t axis, const float position);
float mp_runtime_get_axis_position(uint8_t axis);

float *mp_runtime_get_position();
void mp_runtime_set_position(float position[]);

float mp_runtime_get_work_position(uint8_t axis);
void mp_runtime_set_work_offsets(float offset[]);

stat_t mp_runtime_move_to_target(float target[]);
