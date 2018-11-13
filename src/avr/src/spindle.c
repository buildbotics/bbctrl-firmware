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

#include "spindle.h"
#include "pwm.h"
#include "huanyang.h"
#include "vfd_spindle.h"
#include "stepper.h"
#include "config.h"
#include "command.h"
#include "exec.h"
#include "util.h"

#include <math.h>


typedef struct {
  float dist;
  float speed;
} sync_speed_t;


static struct {
  spindle_type_t type;
  float override;
  sync_speed_t sync_speed;
  float speed;
  bool reversed;
  float min_rpm;
  float max_rpm;

  bool dynamic_power;
  float inv_feed;

  bool dirty;
  spindle_type_t next_type;

} spindle = {
  .type = SPINDLE_TYPE_DISABLED,
  .override = 1,
  .sync_speed = {-1, 0}
};


static float _speed_to_power(float speed) {
  speed *= spindle.override;

  bool negative = speed < 0;
  if (fabs(speed) < spindle.min_rpm) speed = 0;
  else if (spindle.max_rpm <= fabs(speed)) speed = negative ? -1 : 1;
  else speed /= spindle.max_rpm;

  return spindle.reversed ? -speed : speed;
}


static power_update_t _get_update() {
  float power = _speed_to_power(spindle.speed);
  if (spindle.dynamic_power)
    power *= spindle.inv_feed ? spindle.inv_feed * exec_get_velocity() : 1;
  return pwm_get_update(power);
}


static void _set_speed(float speed) {
  spindle.speed = speed;
  spindle.dirty = false;

  if (spindle.type == SPINDLE_TYPE_PWM) {
    // PWM speed updates must be synchronized with stepper movement
    power_update_t update = _get_update();
    spindle_update(update);
    return;
  }

  float power = _speed_to_power(speed);

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_HUANYANG: huanyang_set(power); break;
  default: vfd_spindle_set(power); break;
  }
}


static void _deinit_cb() {
  spindle.type = spindle.next_type;
  spindle.next_type = SPINDLE_TYPE_DISABLED;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: pwm_init(); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_init(); break;
  default: vfd_spindle_init(); break;
  }

  spindle.dirty = true;
}


static void _set_type(spindle_type_t type) {
  if (type == spindle.type) return;

  spindle_type_t old_type = spindle.type;
  spindle.next_type = type;
  spindle.type = SPINDLE_TYPE_DISABLED;

  switch (old_type) {
  case SPINDLE_TYPE_DISABLED: _deinit_cb(); break;
  case SPINDLE_TYPE_PWM: pwm_deinit(_deinit_cb); break;
  case SPINDLE_TYPE_HUANYANG: huanyang_deinit(_deinit_cb); break;
  default: vfd_spindle_deinit(_deinit_cb); break;
  }
}


spindle_type_t spindle_get_type() {return spindle.type;}


void spindle_load_power_updates(power_update_t updates[], float minD,
                                float maxD) {
  float stepD = (maxD - minD) * (1.0 / POWER_MAX_UPDATES);
  float d = minD + 1e-3; // Starting distance

  for (unsigned i = 0; i < POWER_MAX_UPDATES; i++) {
    bool set = false;

    d += stepD; // Ending distance for this power step
    updates[i].state = POWER_IGNORE;

    while (true) {
      // Load new sync speed if needed and available
      if (spindle.sync_speed.dist < 0 && command_peek() == COMMAND_sync_speed)
        spindle.sync_speed = *(sync_speed_t *)(command_next() + 1);

      // Exit if we don't have a speed or it's not ready to be set
      if (spindle.sync_speed.dist < 0 || d < spindle.sync_speed.dist) break;

      set = true;
      spindle.speed = spindle.sync_speed.speed;
      spindle.sync_speed.dist = -1; // Mark done
    }

    // Prep power update
    if (spindle.type == SPINDLE_TYPE_PWM) updates[i] = _get_update();
    else if (set) _set_speed(spindle.speed); // Set speed now for non-PWM
  }
}


void spindle_update(power_update_t update) {return pwm_update(update);}


static void _flush_sync_speeds() {
  spindle.sync_speed.dist = -1;
  while (command_peek() == COMMAND_sync_speed) command_next();
}


// Called from lo-level stepper interrupt
void spindle_idle() {
  if (spindle.dirty) _set_speed(spindle.speed);
  _flush_sync_speeds(); // Flush speeds in case we are holding there are more
}


float spindle_get_speed() {
  float speed = 0;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;
  case SPINDLE_TYPE_PWM: speed = pwm_get(); break;
  case SPINDLE_TYPE_HUANYANG: speed = huanyang_get(); break;
  default: speed = vfd_spindle_get(); break;
  }

  return speed * spindle.max_rpm;
}


void spindle_stop() {
  _flush_sync_speeds();
  _set_speed(0);
}


void spindle_estop() {_set_type(SPINDLE_TYPE_DISABLED);}
bool spindle_is_reversed() {return spindle.reversed;}


// Var callbacks
uint8_t get_tool_type() {return spindle.type;}
void set_tool_type(uint8_t value) {_set_type((spindle_type_t)value);}
float get_speed() {return spindle_get_speed();}
bool get_tool_reversed() {return spindle.reversed;}


void set_tool_reversed(bool reversed) {
  if (spindle.reversed != reversed) {
    spindle.reversed = reversed;
    spindle.dirty = true;
  }
}


float get_max_spin() {return spindle.max_rpm;}
void set_max_spin(float value) {spindle.max_rpm = value; spindle.dirty = true;}
float get_min_spin() {return spindle.min_rpm;}
void set_min_spin(float value) {spindle.min_rpm = value; spindle.dirty = true;}
uint16_t get_speed_override() {return spindle.override * 1000;}


void set_speed_override(uint16_t value) {
  spindle.override = value / 1000.0;
  spindle.dirty = true;
}


bool get_dynamic_power() {return spindle.dynamic_power;}


void set_dynamic_power(bool enable) {
  if (spindle.dynamic_power == enable) return;
  spindle.dynamic_power = enable;
  spindle.dirty = true;
}


float get_inverse_feed() {return spindle.inv_feed;}


void set_inverse_feed(float iF) {
  if (spindle.inv_feed == iF) return;
  spindle.inv_feed = iF;
  spindle.dirty = true;
}


// Command callbacks
stat_t command_sync_speed(char *cmd) {
  sync_speed_t s;

  cmd++; // Skip command code

  // Get distance and speed
  if (!decode_float(&cmd, &s.dist)) return STAT_BAD_FLOAT;
  if (!decode_float(&cmd, &s.speed)) return STAT_BAD_FLOAT;

  // Queue
  command_push(COMMAND_sync_speed, &s);

  return STAT_OK;
}


unsigned command_sync_speed_size() {return sizeof(sync_speed_t);}


void command_sync_speed_exec(void *data) {
  spindle.sync_speed.dist = -1; // Flush any left over
  _set_speed(((sync_speed_t *)data)->speed);
}


stat_t command_speed(char *cmd) {
  cmd++; // Skip command code

  // Get speed
  float speed;
  if (!decode_float(&cmd, &speed)) return STAT_BAD_FLOAT;

  // Queue
  command_push(COMMAND_speed, &speed);

  return STAT_OK;
}


unsigned command_speed_size() {return sizeof(float);}
void command_speed_exec(void *data) {_set_speed(*(float *)data);}
