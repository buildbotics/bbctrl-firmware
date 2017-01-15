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

#include "motor.h"
#include "config.h"
#include "hardware.h"
#include "cpp_magic.h"
#include "rtc.h"
#include "report.h"
#include "stepper.h"
#include "drv8711.h"
#include "estop.h"
#include "gcode_state.h"
#include "axis.h"
#include "util.h"

#include "plan/runtime.h"
#include "plan/calibrate.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


typedef enum {
  MOTOR_OFF,
  MOTOR_IDLE,                    // motor stopped and may be partially energized
  MOTOR_ENERGIZING,
  MOTOR_ACTIVE
} motor_power_state_t;


typedef struct {
  // Config
  uint8_t axis;                  // map motor to axis
  uint16_t microsteps;           // microsteps per full step
  bool reverse;
  motor_power_mode_t power_mode;
  float step_angle;              // degrees per whole step
  float travel_rev;              // mm or deg of travel per motor revolution
  uint8_t step_pin;
  uint8_t dir_pin;
  TC0_t *timer;
  DMA_CH_t *dma;
  uint8_t dma_trigger;

  // Runtime state
  motor_power_state_t power_state; // state machine for managing motor power
  uint32_t timeout;
  motor_flags_t flags;
  int32_t encoder;
  int32_t commanded;
  int32_t steps;                 // Currently used by the x-axis only
  uint8_t last_clock;
  bool wasEnabled;
  int32_t error;
  bool last_negative;            // Last step sign
  bool reading;

  // Move prep
  uint8_t timer_clock;           // clock divisor setting or zero for off
  uint16_t timer_period;         // clock period counter
  bool negative;                 // step sign
  direction_t direction;         // travel direction corrected for polarity
  int32_t position;
  bool round_up;                 // toggle rounding direction
  float power;
} motor_t;


static motor_t motors[MOTORS] = {
  {
    .axis            = AXIS_X,
    .step_pin        = STEP_X_PIN,
    .dir_pin         = DIR_X_PIN,
    .timer           = &M1_TIMER,
    .dma             = &M1_DMA_CH,
    .dma_trigger     = M1_DMA_TRIGGER,
  }, {
    .axis            = AXIS_Y,
    .step_pin        = STEP_Y_PIN,
    .dir_pin         = DIR_Y_PIN,
    .timer           = &M2_TIMER,
    .dma             = &M2_DMA_CH,
    .dma_trigger     = M2_DMA_TRIGGER,
  }, {
    .axis            = AXIS_Z,
    .step_pin        = STEP_Z_PIN,
    .dir_pin         = DIR_Z_PIN,
    .timer           = &M3_TIMER,
    .dma             = &M3_DMA_CH,
    .dma_trigger     = M3_DMA_TRIGGER,
  }, {
    .axis            = AXIS_A,
    .step_pin        = STEP_A_PIN,
    .dir_pin         = DIR_A_PIN,
    .timer           = (TC0_t *)&M4_TIMER,
    .dma             = &M4_DMA_CH,
    .dma_trigger     = M4_DMA_TRIGGER,
    }
};


static uint8_t _dummy;


void motor_init() {
  // Enable DMA
  DMA.CTRL = DMA_RESET_bm;
  DMA.CTRL = DMA_ENABLE_bm;
  DMA.INTFLAGS = 0xff; // clear all interrupts

  // Motor enable
  OUTSET_PIN(MOTOR_ENABLE_PIN); // Low (disabled)
  DIRSET_PIN(MOTOR_ENABLE_PIN); // Output

  for (int motor = 0; motor < MOTORS; motor++) {
    motor_t *m = &motors[motor];

    axis_set_motor(m->axis, motor);

    // IO pins
    DIRSET_PIN(m->step_pin);   // Output
    DIRSET_PIN(m->dir_pin);    // Output

    // Setup motor timer
    m->timer->CTRLB = TC_WGMODE_FRQ_gc | TC1_CCAEN_bm;

    // Setup DMA channel as timer event counter
    m->dma->ADDRCTRL = DMA_CH_SRCDIR_FIXED_gc | DMA_CH_DESTDIR_FIXED_gc;
    m->dma->TRIGSRC = m->dma_trigger;
    m->dma->REPCNT = 0;

    // Note, the DMA transfer must read CCA to clear the trigger
    m->dma->SRCADDR0 = (((uintptr_t)&m->timer->CCA) >> 0) & 0xff;
    m->dma->SRCADDR1 = (((uintptr_t)&m->timer->CCA) >> 8) & 0xff;
    m->dma->SRCADDR2 = 0;

    m->dma->DESTADDR0 = (((uintptr_t)&_dummy) >> 0) & 0xff;
    m->dma->DESTADDR1 = (((uintptr_t)&_dummy) >> 8) & 0xff;
    m->dma->DESTADDR2 = 0;

    m->dma->CTRLB = 0;
    m->dma->CTRLA =
      DMA_CH_REPEAT_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;
    m->dma->CTRLA |= DMA_CH_ENABLE_bm;

    drv8711_set_microsteps(motor, m->microsteps);
  }
}


void motor_enable(int motor, bool enable) {
  if (enable) OUTSET_PIN(MOTOR_ENABLE_PIN); // Active high
  else {
    OUTCLR_PIN(MOTOR_ENABLE_PIN);
    motors[motor].power_state = MOTOR_DISABLED;
  }
}


bool motor_is_enabled(int motor) {
  return motors[motor].power_mode != MOTOR_DISABLED;
}


int motor_get_axis(int motor) {return motors[motor].axis;}


void motor_set_stall_callback(int motor, stall_callback_t cb) {
  drv8711_set_stall_callback(motor, cb);
}


/// @return computed homing velocity
float motor_get_stall_homing_velocity(int motor) {
  // Compute velocity:
  //   velocity = travel_rev * step_angle * 60 / (SMPLTH * mstep * 360 * 2)
  //   SMPLTH = 50us = 0.00005s
  //   mstep = 8
  return motors[motor].travel_rev * motors[motor].step_angle * 1667 /
    motors[motor].microsteps;
}


float motor_get_steps_per_unit(int motor) {
  return 360 * motors[motor].microsteps / motors[motor].travel_rev /
    motors[motor].step_angle;
}


float motor_get_units_per_step(int motor) {
  return motors[motor].travel_rev * motors[motor].step_angle /
    motors[motor].microsteps / 360;
}


float _get_max_velocity(int motor) {
  return
    axis_get_velocity_max(motors[motor].axis) * motor_get_steps_per_unit(motor);
}


uint16_t motor_get_microsteps(int motor) {return motors[motor].microsteps;}


void motor_set_microsteps(int motor, uint16_t microsteps) {
  switch (microsteps) {
  case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
    break;
  default: return;
  }

  motors[motor].microsteps = microsteps;
  drv8711_set_microsteps(motor, microsteps);
}


int32_t motor_get_encoder(int motor) {return motors[motor].encoder;}


void motor_set_encoder(int motor, float encoder) {
  //if (st_is_busy()) ALARM(STAT_INTERNAL_ERROR); TODO

  motor_t *m = &motors[motor];
  m->encoder = m->position = m->commanded = round(encoder);
  m->error = 0;
}


int32_t motor_get_error(int motor) {
  motor_t *m = &motors[motor];

  m->reading = true;
  int32_t error = motors[motor].error;
  m->reading = false;

  return error;
}


int32_t motor_get_position(int motor) {return motors[motor].position;}


/// returns true if motor is in an error state
bool motor_error(int m) {
  uint8_t flags = motors[m].flags;
  if (calibrate_busy()) flags &= ~MOTOR_FLAG_STALLED_bm;
  return flags & MOTOR_FLAG_ERROR_bm;
}


bool motor_stalled(int m) {
  return motors[m].flags & MOTOR_FLAG_STALLED_bm;
}


void motor_reset(int m) {
  motors[m].flags = 0;
}


static void _set_power_state(int motor, motor_power_state_t state) {
  motors[motor].power_state = state;
  report_request();
}


static void _update_power(int motor) {
  motor_t *m = &motors[motor];

  switch (m->power_mode) {
  case MOTOR_POWERED_ONLY_WHEN_MOVING:
  case MOTOR_POWERED_IN_CYCLE:
    if (rtc_expired(m->timeout)) {
      if (m->power_state == MOTOR_ACTIVE) {
        _set_power_state(motor, MOTOR_IDLE);
        drv8711_set_state(motor, DRV8711_IDLE);
      }
      break;
    }
    // Fall through

  case MOTOR_ALWAYS_POWERED:
    if (m->power_state != MOTOR_ACTIVE && m->power_state != MOTOR_ENERGIZING &&
        !motor_error(motor)) {
      _set_power_state(motor, MOTOR_ENERGIZING);
      // TODO is ~5ms enough time to enable the motor?
      drv8711_set_state(motor, DRV8711_ACTIVE);

      motor_driver_callback(motor); // TODO Shouldn't call this directly
    }
    break;

  default: // Disabled
    if (m->power_state != MOTOR_OFF) {
      _set_power_state(motor, MOTOR_OFF);
      drv8711_set_state(motor, DRV8711_DISABLED);
    }
  }
}


bool motor_energizing() {
  for (int m = 0; m < MOTORS; m++)
    if (motors[m].power_state == MOTOR_ENERGIZING)
      return true;

  return false;
}


void motor_driver_callback(int motor) {
  motor_t *m = &motors[motor];

  if (m->power_state == MOTOR_IDLE) m->flags &= ~MOTOR_FLAG_ENABLED_bm;
  else if (!estop_triggered()) {
    m->power_state = MOTOR_ACTIVE;
    m->flags |= MOTOR_FLAG_ENABLED_bm;
    motor_enable(motor, true);
  }

  report_request();
}


/// Callback to manage motor power sequencing and power-down timing.
stat_t motor_rtc_callback() { // called by controller
  for (int motor = 0; motor < MOTORS; motor++)
    _update_power(motor);

  return STAT_OK;
}


void motor_error_callback(int motor, motor_flags_t errors) {
  motor_t *m = &motors[motor];

  if (m->power_state != MOTOR_ACTIVE) return;

  m->flags |= errors & MOTOR_FLAG_ERROR_bm;
  report_request();

  if (motor_error(motor)) {
    if (m->flags & MOTOR_FLAG_STALLED_bm) ALARM(STAT_MOTOR_STALLED);
    if (m->flags & MOTOR_FLAG_OVER_TEMP_bm) ALARM(STAT_MOTOR_OVER_TEMP);
    if (m->flags & MOTOR_FLAG_OVER_CURRENT_bm) ALARM(STAT_MOTOR_OVER_CURRENT);
    if (m->flags & MOTOR_FLAG_DRIVER_FAULT_bm) ALARM(STAT_MOTOR_DRIVER_FAULT);
    if (m->flags & MOTOR_FLAG_UNDER_VOLTAGE_bm) ALARM(STAT_MOTOR_UNDER_VOLTAGE);
  }
}


void motor_load_move(int motor) {
  motor_t *m = &motors[motor];

  // Get actual step count from DMA channel
  uint16_t steps = 0xffff - m->dma->TRFCNT;
  m->dma->TRFCNT = 0xffff; // Reset DMA channel counter
  m->dma->CTRLB = DMA_CH_CHBUSY_bm | DMA_CH_CHPEND_bm;
  m->dma->CTRLA |= DMA_CH_ENABLE_bm;

  // Adjust clock count
  if (m->last_clock) {
    uint32_t count = m->timer->CNT;
    int8_t freq_change = m->last_clock - m->timer_clock;

    count <<= freq_change; // Adjust count

    if (m->timer_period < count) count -= m->timer_period;
    if (m->timer_period < count) count -= m->timer_period;
    if (m->timer_period < count) count = m->timer_period / 2;

    m->timer->CNT = count;

  } else m->timer->CNT = m->timer_period / 2;

  // Set or zero runtime clock and period
  m->timer->CCA = m->timer_period;     // Set frequency
  m->timer->CTRLA = m->timer_clock;    // Start or stop
  m->last_clock = m->timer_clock;
  m->timer_clock = 0;                  // Clear clock

  // Set direction
  if (m->direction == DIRECTION_CW) OUTCLR_PIN(m->dir_pin);
  else OUTSET_PIN(m->dir_pin);

  // Accumulate encoder
  if (!m->wasEnabled) steps = 0;

  m->encoder += m->last_negative ? -(int32_t)steps : steps;
  m->last_negative = m->negative;

  // Compute error
  if (!m->reading) m->error = m->commanded - m->encoder;
  m->commanded = m->position;

  // Set power
  drv8711_set_power(motor, m->power);
}


void motor_end_move(int motor) {
  motor_t *m = &motors[motor];
  m->wasEnabled = m->dma->CTRLA & DMA_CH_ENABLE_bm;
  m->dma->CTRLA &= ~DMA_CH_ENABLE_bm;
  m->timer->CTRLA = 0; // Stop clock
}


stat_t motor_prep_move(int motor, int32_t clocks, float target, int32_t error,
                       float time) {
  motor_t *m = &motors[motor];

  // Validate input
  if (motor < 0 || MOTORS < motor) return ALARM(STAT_INTERNAL_ERROR);
  if (clocks < 0)    return ALARM(STAT_INTERNAL_ERROR);
  if (isinf(target)) return ALARM(STAT_MOVE_TARGET_INFINITE);
  if (isnan(target)) return ALARM(STAT_MOVE_TARGET_NAN);

  // Compute motor timer clock and period. Rounding is performed to eliminate
  // a negative bias in the uint32_t conversion that results in long-term
  // negative drift.
  int32_t travel = round(target) - m->position + error;
  uint32_t ticks_per_step = travel ? labs(clocks / 2 / travel) : 0;
  m->position = round(target);

  // Setup the direction, compensating for polarity.
  m->negative = travel < 0;
  if (m->negative) m->direction = DIRECTION_CCW ^ m->reverse;
  else m->direction = DIRECTION_CW ^ m->reverse;

  // Find the clock rate that will fit the required number of steps
  if (ticks_per_step <= 0xffff) m->timer_clock = TC_CLKSEL_DIV1_gc;
  else if (ticks_per_step <= 0x1ffff) m->timer_clock = TC_CLKSEL_DIV2_gc;
  else if (ticks_per_step <= 0x3ffff) m->timer_clock = TC_CLKSEL_DIV4_gc;
  else if (ticks_per_step <= 0x7ffff) m->timer_clock = TC_CLKSEL_DIV8_gc;
  else m->timer_clock = 0; // Clock off, too slow

  // Note, we rely on the fact that TC_CLKSEL_DIV1_gc through TC_CLKSEL_DIV8_gc
  // equal 1, 2, 3 & 4 respectively.
  m->timer_period = ticks_per_step >> (m->timer_clock - 1);

  // Round up if DIV4 or DIV8 and the error is high enough
  if (0xffff < ticks_per_step && m->timer_period < 0xffff) {
    int8_t step_error = ticks_per_step & ((1 << (m->timer_clock - 1)) - 1);
    int8_t half_error = 1 << (m->timer_clock - 2);

    if (step_error == half_error) {
      if (m->round_up) m->timer_period++;
      m->round_up = !m->round_up;

    } else if (half_error < step_error) m->timer_period++;
  }

  if (!m->timer_period) m->timer_clock = 0;
  if (!m->timer_clock) m->timer_period = 0;

  // Compute power from axis velocity
  m->power = travel / (_get_max_velocity(motor) * time);

  // Power motor
  switch (m->power_mode) {
  case MOTOR_POWERED_ONLY_WHEN_MOVING:
    if (!m->timer_clock) break; // Not moving
    // Fall through

  case MOTOR_ALWAYS_POWERED: case MOTOR_POWERED_IN_CYCLE:
    // Reset timeout
    m->timeout = rtc_get_time() + MOTOR_IDLE_TIMEOUT * 1000;
    break;

  default: break;
  }
  _update_power(motor);

  return STAT_OK;
}


// Var callbacks
char get_axis_name(int motor) {return axis_get_char(motors[motor].axis);}


void set_axis_name(int motor, char axis) {
  int id = axis_get_id(axis);
  if (id != -1) motors[motor].axis = id;
}


float get_step_angle(int motor) {return motors[motor].step_angle;}
void set_step_angle(int motor, float value) {motors[motor].step_angle = value;}
float get_travel(int motor) {return motors[motor].travel_rev;}
void set_travel(int motor, float value) {motors[motor].travel_rev = value;}
uint16_t get_microstep(int motor) {return motors[motor].microsteps;}


void set_microstep(int motor, uint16_t value) {
  if (motor < 0 || MOTORS <= motor) return;
  motor_set_microsteps(motor, value);
}


bool get_reverse(int motor) {
  if (motor < 0 || MOTORS <= motor) return 0;
  return motors[motor].reverse;
}


void set_reverse(int motor, bool value) {motors[motor].reverse = value;}
uint8_t get_motor_axis(int motor) {return motors[motor].axis;}


void set_motor_axis(int motor, uint8_t value) {
  if (MOTORS <= motor || AXES <= value || value == motors[motor].axis) return;
  axis_set_motor(motors[motor].axis, -1);
  motors[motor].axis = value;
  axis_set_motor(value, motor);
}


uint8_t get_power_mode(int motor) {return motors[motor].power_mode;}


void set_power_mode(int motor, uint8_t value) {
  if (value < MOTOR_POWER_MODE_MAX_VALUE)
    motors[motor].power_mode = value;
}


uint8_t get_status_flags(int motor) {return motors[motor].flags;}


void print_status_flags(uint8_t flags) {
  bool first = true;

  putchar('"');

  if (MOTOR_FLAG_ENABLED_bm & flags) {
    printf_P(PSTR("enable"));
    first = false;
  }

  if (MOTOR_FLAG_STALLED_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("stall"));
    first = false;
  }

  if (MOTOR_FLAG_OVER_TEMP_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("over temp"));
    first = false;
  }

  if (MOTOR_FLAG_OVER_CURRENT_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("over current"));
    first = false;
  }

  if (MOTOR_FLAG_DRIVER_FAULT_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("fault"));
    first = false;
  }

  if (MOTOR_FLAG_UNDER_VOLTAGE_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("uvlo"));
    first = false;
  }

  putchar('"');
}


uint8_t get_status_strings(int m) {return get_status_flags(m);}


// Command callback
void command_mreset(int argc, char *argv[]) {
  if (argc == 1)
    for (int m = 0; m < MOTORS; m++)
      motor_reset(m);

  else {
    int m = atoi(argv[1]);
    if (m < MOTORS) motor_reset(m);
  }

  report_request();
}
