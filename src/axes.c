/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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


#include "canonical_machine.h"


uint8_t get_axis_mode(int axis) {
  return cm.a[axis].axis_mode;
}


void set_axis_mode(int axis, uint8_t value) {
  if (value < AXIS_MODE_MAX)
    cm.a[axis].axis_mode = value;
}


float get_max_velocity(int axis) {
  return cm.a[axis].velocity_max;
}


void set_max_velocity(int axis, float value) {
  cm.a[axis].velocity_max = value;
}


float get_max_feedrate(int axis) {
  return cm.a[axis].feedrate_max;
}


void set_max_feedrate(int axis, float value) {
  cm.a[axis].feedrate_max = value;
}


float get_max_jerk(int axis) {
  return cm.a[axis].jerk_max;
}


void set_max_jerk(int axis, float value) {
  cm.a[axis].jerk_max = value;
}


float get_junction_dev(int axis) {
  return cm.a[axis].junction_dev;
}


void set_junction_dev(int axis, float value) {
  cm.a[axis].junction_dev = value;
}


float get_travel_min(int axis) {
  return cm.a[axis].travel_min;
}


void set_travel_min(int axis, float value) {
  cm.a[axis].travel_min = value;
}


float get_travel_max(int axis) {
  return cm.a[axis].travel_max;
}


void set_travel_max(int axis, float value) {
  cm.a[axis].travel_max = value;
}


float get_jerk_homing(int axis) {
  return cm.a[axis].jerk_homing;
}


void set_jerk_homing(int axis, float value) {
  cm.a[axis].jerk_homing = value;
}


float get_search_vel(int axis) {
  return cm.a[axis].search_velocity;
}


void set_search_vel(int axis, float value) {
  cm.a[axis].search_velocity = value;
}


float get_latch_vel(int axis) {
  return cm.a[axis].latch_velocity;
}


void set_latch_vel(int axis, float value) {
  cm.a[axis].latch_velocity = value;
}


float get_latch_backoff(int axis) {
  return cm.a[axis].latch_backoff;
}


void set_latch_backoff(int axis, float value) {
  cm.a[axis].latch_backoff = value;
}


float get_zero_backoff(int axis) {
  return cm.a[axis].zero_backoff;
}


void set_zero_backoff(int axis, float value) {
  cm.a[axis].zero_backoff = value;
}







// TODO fix these
uint8_t get_min_switch(int axis) {
  //return cm.a[axis].min_switch;
  return 0;
}


void set_min_switch(int axis, uint8_t value) {
  //cm.a[axis].min_switch = value;
}


uint8_t get_max_switch(int axis) {
  //return cm.a[axis].max_switch;
  return 0;
}


void set_max_switch(int axis, uint8_t value) {
  //cm.a[axis].max_switch = value;
}
