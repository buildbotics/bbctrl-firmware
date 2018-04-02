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


#include "config.h"
#include "status.h"

#include <stdbool.h>
#include <stdint.h>


typedef stat_t (*exec_cb_t)();


void exec_init();

void exec_get_position(float p[AXES]);
float exec_get_axis_position(int axis);
void exec_set_velocity(float v);
float exec_get_velocity();
void exec_set_acceleration(float a);
float exec_get_acceleration();
void exec_set_jerk(float j);

void exec_set_cb(exec_cb_t cb);

void exec_move_to_target(float time, const float target[]);
stat_t exec_next();
