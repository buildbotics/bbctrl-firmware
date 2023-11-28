/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2023, Buildbotics LLC, All rights reserved.

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

#include "spindle.h"
#include "pwm.h"
#include "huanyang.h"
#include "vfd_spindle.h"
#include "stepper.h"
#include "config.h"
#include "command.h"
#include "exec.h"
#include "estop.h"
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
  float inv_max_rpm;

  bool dynamic_power;
  float inv_feed;

  spindle_type_t next_type;

} spindle = {
  .type = SPINDLE_TYPE_DISABLED,
  .override = 1,
  .sync_speed = {-1, 0}
};


static float _get_power() {
  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: return 0;
  case SPINDLE_TYPE_PWM:      return pwm_get();
  case SPINDLE_TYPE_HUANYANG: return huanyang_get();
  default:                    return vfd_spindle_get();
  }
}


static float _speed_to_power(float speed) {
  bool negative = speed < 0;
  float power = fabs(speed * spindle.override);

  if (power < spindle.min_rpm) power = 0;
  else if (spindle.max_rpm <= power) power = 1;
  else power *= spindle.inv_max_rpm;

  return (negative ^ spindle.reversed) ? -power : power;
}


static void _set_speed(float speed) {
  spindle.speed = speed;

  float power = _speed_to_power(speed);

  if (estop_triggered()) power = 0;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: break;

  case SPINDLE_TYPE_PWM: {
    // PWM speed updates must be synchronized with stepper movement
    spindle.sync_speed.dist = 0;
    spindle.sync_speed.speed = speed;
    break;
  }

  case SPINDLE_TYPE_HUANYANG: huanyang_set(power); break;
  default: vfd_spindle_set(power); break;
  }
}


static void _deinit_cb() {
  spindle.type = spindle.next_type;
  spindle.next_type = SPINDLE_TYPE_DISABLED;

  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED:                     break;
  case SPINDLE_TYPE_PWM:      pwm_init();         break;
  case SPINDLE_TYPE_HUANYANG: huanyang_init();    break;
  default:                    vfd_spindle_init(); break;
  }

  spindle_update_speed();
}


static void _set_type(spindle_type_t type) {
  if (type == spindle.type) return;

  spindle_type_t old_type = spindle.type;
  spindle.next_type = type;
  spindle.type = SPINDLE_TYPE_DISABLED;

  switch (old_type) {
  case SPINDLE_TYPE_DISABLED: _deinit_cb();                   break;
  case SPINDLE_TYPE_PWM:      pwm_deinit(_deinit_cb);         break;
  case SPINDLE_TYPE_HUANYANG: huanyang_deinit(_deinit_cb);    break;
  default:                    vfd_spindle_deinit(_deinit_cb); break;
  }
}


spindle_type_t spindle_get_type() {return spindle.type;}


static power_update_t _get_power_update() {
  float power = _speed_to_power(spindle.speed);

  // Handle dynamic power
  if (spindle.type == SPINDLE_TYPE_PWM && spindle.dynamic_power &&
      spindle.inv_feed) {
    float scale = spindle.inv_feed * exec_get_velocity();
    if (scale < 1) power *= scale;
  }

  return pwm_get_update(power);
}


void spindle_load_power_updates(power_update_t updates[], float minD,
                                float maxD) {
  float stepD = (maxD - minD) * (1.0 / POWER_MAX_UPDATES);
  float d = minD + 1e-3; // Starting distance

  for (unsigned i = 0; i < POWER_MAX_UPDATES; i++) {
    bool changed = false;
    d += stepD; // Ending distance for this power step

    while (true) {
      // Load new sync speed if needed and available
      if (spindle.sync_speed.dist < 0 && command_peek() == COMMAND_sync_speed)
        spindle.sync_speed = *(sync_speed_t *)(command_next() + 1);

      // Exit if we don't have a speed or it's not ready to be set
      if (spindle.sync_speed.dist == -1 || d < spindle.sync_speed.dist) break;

      // Load sync speed
      spindle.sync_speed.dist = -1; // Mark done
      spindle.speed = spindle.sync_speed.speed;
      changed = true;
    }

    if (spindle.type == SPINDLE_TYPE_PWM) updates[i] = _get_power_update();
    else {
      updates[i].state = POWER_IGNORE;
      if (changed) spindle_update_speed();
    }
  }
}


// Called from hi-priority stepper interrupt
void spindle_update(const power_update_t &update) {pwm_update(update);}
void spindle_update_speed() {_set_speed(spindle.speed);}


// Called from lo-priority stepper interrupt
void spindle_idle() {
  if (spindle.sync_speed.dist != -1) {
    spindle.sync_speed.dist = -1; // Mark done
    spindle.speed = spindle.sync_speed.speed;

    if (spindle.type == SPINDLE_TYPE_PWM) spindle_update(_get_power_update());
    else spindle_update_speed();
  }
}


void spindle_stop() {_set_speed(0);} // Only called when steppers have halted


void spindle_estop() {
  _set_speed(0);
  if (spindle.type == SPINDLE_TYPE_PWM) pwm_update(pwm_get_update(0));
}


// Var callbacks
uint8_t get_tool_type() {return spindle.type;}
void set_tool_type(uint8_t value) {_set_type((spindle_type_t)value);}
bool get_tool_reversed() {return spindle.reversed;}


void set_tool_reversed(bool reversed) {
  if (spindle.reversed == reversed) return;
  spindle.reversed = reversed;
  spindle_update_speed();
}


float get_speed() {return _get_power() * spindle.max_rpm;}
float get_max_spin() {return spindle.max_rpm;}


void set_max_spin(float value) {
  if (spindle.max_rpm != value) {
    spindle.max_rpm = value;
    spindle.inv_max_rpm = 1 / value;
    spindle_update_speed();
  }
}


float get_min_spin() {return spindle.min_rpm;}


void set_min_spin(float value) {
  if (spindle.min_rpm != value) {
    spindle.min_rpm = value;
    spindle_update_speed();
  }
}


uint16_t get_spindle_status() {
  switch (spindle.type) {
  case SPINDLE_TYPE_DISABLED: return 0;
  case SPINDLE_TYPE_PWM:      return 0;
  case SPINDLE_TYPE_HUANYANG: return huanyang_get_status();
  default:                    return vfd_get_status();
  }
}


float get_speed_override() {return spindle.override;}


void set_speed_override(float value) {
  if (spindle.override != value) {
    spindle.override = value;
    spindle_update_speed();
  }
}


bool get_dynamic_power() {return spindle.dynamic_power;}


void set_dynamic_power(bool enable) {
  if (spindle.dynamic_power != enable) {
    spindle.dynamic_power = enable;
    spindle_update_speed();
  }
}


float get_inverse_feed() {return spindle.inv_feed;}


void set_inverse_feed(float iF) {
  if (spindle.inv_feed != iF) {
    spindle.inv_feed = iF;
    spindle_update_speed();
  }
}


// Command callbacks
stat_t command_sync_speed(char *cmd) {
  sync_speed_t s;

  cmd++; // Skip command code

  // Get distance and speed
  if (!decode_float(&cmd, &s.dist) || s.dist < 0) return STAT_BAD_FLOAT;
  if (!decode_float(&cmd, &s.speed))              return STAT_BAD_FLOAT;

  // Queue
  command_push(COMMAND_sync_speed, &s);

  return STAT_OK;
}


unsigned command_sync_speed_size() {return sizeof(sync_speed_t);}


void command_sync_speed_exec(void *data) {
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
