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

#include "status.h"
#include "spindle.h"
#include "i2c.h"
#include "type.h"
#include "switch.h"
#include "cpp_magic.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>


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


float square(float x) {return x * x;}
void i2c_set_read_callback(i2c_read_cb_t cb) {}
void print_status_flags(uint16_t flags) {DEBUG_CALL();}


bool estop = false;


void estop_trigger(stat_t reason) {
  DEBUG_CALL("%s", status_to_pgmstr(reason));
  estop = true;
  abort();
}


void estop_clear() {DEBUG_CALL(); estop = false;}
bool estop_triggered() {return estop;}


void hw_request_hard_reset() {DEBUG_CALL(); exit(0);}


bool usart_tx_fill() {return 0;}


char *usart_readline() {
  static char *cmd = 0;

  if (cmd) {
    free(cmd);
    cmd = 0;
  }

  size_t n = 0;
  if (getline(&cmd, &n, stdin) == -1) {
    free(cmd);
    return 0;
  }

  n = strlen(cmd);
  while (n && isspace(cmd[n - 1])) cmd[--n] = 0;

  return cmd;
}


void spindle_set_speed(float speed) {DEBUG_CALL("%f", speed);}
void spindle_stop() {}


void st_set_position(int32_t position[AXES]) {
  DEBUG_CALL("%d, %d, %d, %d",
             position[0], position[1], position[2], position[3]);
}


bool switch_is_active(switch_id_t sw) {DEBUG_CALL("%d", sw); return false;}
bool switch_is_enabled(switch_id_t sw) {DEBUG_CALL("%d", sw); return false;}
void switch_set_callback(switch_id_t sw, switch_callback_t cb) {}


static uint32_t ticks = 0;
uint32_t rtc_get_time() {return ticks;}
bool rtc_expired(uint32_t t) {return true;}


bool motor_is_enabled(int motor) {return true;}
int motor_get_axis(int motor) {return motor;}
bool motor_get_homed(int motor) {return false;}
float motor_get_soft_limit(int motor, bool min) {return 0;}
void motor_set_position(int motor, float position) {}

#define MICROSTEPS 32
#define TRAVEL_REV 5
#define STEP_ANGLE 1.8


int32_t motor_get_encoder(int motor) {DEBUG_CALL("%d", motor); return 0;}
void motor_end_move(int motor) {DEBUG_CALL("%d", motor);}
int32_t motor_get_error(int motor) {return 0;}


bool st_is_busy() {return false;}


stat_t st_prep_line(float time, const float target[]) {
  ASSERT(isfinite(time));

  const float maxTarget = 1000;
  for (int i = 0; i < 3; i++) {
    ASSERT(isfinite(target[i]));
    ASSERT(-maxTarget < target[i] && target[i] < maxTarget);
  }

  printf("%0.10f, %0.10f, %0.10f, %0.10f\n",
         time, target[0], target[1], target[2]);

  return STAT_OK;
}


void st_prep_dwell(float seconds) {DEBUG_CALL("%f", seconds);}


void outputs_stop() {}
void jog_stop() {}


// Command callbacks
stat_t command_estop(char *cmd) {DEBUG_CALL(); return STAT_OK;}
stat_t command_clear(char *cmd) {DEBUG_CALL(); return STAT_OK;}
stat_t command_jog(char *cmd) {DEBUG_CALL(); return STAT_OK;}
stat_t command_input(char *cmd) {DEBUG_CALL(); return STAT_OK;}
unsigned command_input_size() {return 0;}
void command_input_exec(void *data) {}
