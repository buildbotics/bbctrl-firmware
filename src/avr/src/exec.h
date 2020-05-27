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

#pragma once


#include "config.h"
#include "spindle.h"
#include "status.h"

#include <stdbool.h>
#include <stdint.h>


typedef stat_t (*exec_cb_t)();


void exec_init();

void exec_get_position(float p[AXES]);
float exec_get_axis_position(int axis);
float exec_get_power_scale();
void exec_set_velocity(float v);
float exec_get_velocity();
void exec_set_acceleration(float a);
float exec_get_acceleration();
void exec_set_jerk(float j);

void exec_set_cb(exec_cb_t cb);

void exec_move_to_target(const float target[]);
stat_t exec_segment(float time, const float target[], float vel, float accel,
                    float maxAccel, float maxJerk,
                    const power_update_t power_updates[]);
stat_t exec_next();
