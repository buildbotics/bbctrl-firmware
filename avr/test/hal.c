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

#include "status.h"
#include "spindle.h"
#include "i2c.h"
#include "cpp_magic.h"
#include "plan/buffer.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


typedef uint8_t flags_t;
typedef const char *string;
typedef const char *pstring;


#define VAR(NAME, CODE, TYPE, INDEX, SET, ...)                          \
  TYPE get_##NAME(IF(INDEX)(int index)) __attribute__((weak));          \
  TYPE get_##NAME(IF(INDEX)(int index)) {                               \
    DEBUG_CALL();                                                       \
    return 0;                                                           \
  }                                                                     \
  IF(SET)                                                               \
  (void set_##NAME(IF(INDEX)(int index,) TYPE value) __attribute__((weak)); \
   void set_##NAME(IF(INDEX)(int index,) TYPE value) {                  \
     DEBUG_CALL();                                                      \
   })

#include "vars.def"
#undef VAR


void command_mreset(int argc, char *argv[]) {}
void command_home(int argc, char *argv[]) {}
void i2c_set_read_callback(i2c_read_cb_t cb) {}
void print_status_flags(uint8_t flags) {DEBUG_CALL();}
uint8_t hw_disable_watchdog() {return 0;}
void hw_restore_watchdog(uint8_t state) {}


bool estop = false;


void estop_trigger(stat_t reason) {
  DEBUG_CALL("%s", status_to_pgmstr(reason));
  mp_queue_dump();
  estop = true;
  abort();
}


void estop_clear() {
  DEBUG_CALL();
  estop = false;
}


bool estop_triggered() {return estop;}


void hw_request_hard_reset() {
  DEBUG_CALL();
  exit(0);
}


bool usart_tx_empty() {return true;}
bool usart_tx_full() {return false;}


char *usart_readline() {
  static char *cmd = 0;

  if (cmd) {
    free(cmd);
    cmd = 0;
  }

  size_t n = 0;
  if (getline(&cmd, &n, stdin) == -1) {
    free(cmd);
    cmd = 0;
  }

  return cmd;
}


void coolant_init() {}


void coolant_set_mist(bool x) {
  DEBUG_CALL("%s", x ? "true" : "false");
}


void coolant_set_flood(bool x) {
  DEBUG_CALL("%s", x ? "true" : "false");
}


void spindle_init() {}


void spindle_set_speed(float speed) {
  DEBUG_CALL("%f", speed);
}


void spindle_set_mode(spindle_mode_t mode) {
  DEBUG_CALL("%d", mode);
}


void motor_set_position(int motor, int32_t position) {
  DEBUG_CALL("%d, %d", motor, position);
}


bool switch_is_active(int index) {
  DEBUG_CALL("%d", index);
  return false;
}


bool switch_is_enabled(int index) {
  DEBUG_CALL("%d", index);
  return false;
}


static uint32_t ticks = 0;


uint32_t rtc_get_time() {return ticks;}


bool rtc_expired(uint32_t t) {
  return true;
  return 0 <= (int32_t)(ticks - t);
}


bool motor_is_enabled(int motor) {return true;}
int motor_get_axis(int motor) {return motor;}


#define MICROSTEPS 32
#define TRAVEL_REV 5
#define STEP_ANGLE 1.8


float motor_position[MOTORS] = {0};


float motor_get_steps_per_unit(int motor) {
  return 360 * MICROSTEPS / TRAVEL_REV / STEP_ANGLE;
}


int32_t motor_get_encoder(int motor) {
  DEBUG_CALL("%d", motor);
  return 0;
}


void motor_end_move(int motor) {
  DEBUG_CALL("%d", motor);
}


int32_t motor_get_error(int motor) {return 0;}
int32_t motor_get_position(int motor) {return motor_position[motor];}


bool st_is_busy() {return false;}


float square(float x) {return x * x;}


stat_t st_prep_line(float time, const float target[]) {
  DEBUG_CALL("%f, (%f, %f, %f, %f)",
             time, target[0], target[1], target[2], target[3]);

  for (int i = 0; i < MOTORS; i++)
    motor_position[i] = target[i];

  printf("%0.10f, %0.10f, %0.10f, %0.10f\n",
         time, motor_position[0], motor_position[1], motor_position[2]);

  return STAT_OK;
}


void st_prep_dwell(float seconds) {
  DEBUG_CALL("%f", seconds);
}
