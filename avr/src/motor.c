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
#include "pgmspace.h"

#include "plan/runtime.h"
#include "plan/calibrate.h"

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
  uint32_t power_timeout;
  motor_flags_t flags;
  bool active;

  // Move prep
  uint16_t steps;
  bool clockwise;
  int32_t position;
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

    // Note, the DMA transfer must read CCA to clear the trigger
    m->dma->SRCADDR0 = (((uintptr_t)&m->timer->CCA) >> 0) & 0xff;
    m->dma->SRCADDR1 = (((uintptr_t)&m->timer->CCA) >> 8) & 0xff;
    m->dma->SRCADDR2 = 0;

    m->dma->DESTADDR0 = (((uintptr_t)&_dummy) >> 0) & 0xff;
    m->dma->DESTADDR1 = (((uintptr_t)&_dummy) >> 8) & 0xff;
    m->dma->DESTADDR2 = 0;

    m->dma->REPCNT = 0;
    m->dma->CTRLB = DMA_CH_TRNINTLVL_HI_gc;
    m->dma->CTRLA = DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;

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


void motor_set_position(int motor, int32_t position) {
  //if (st_is_busy()) ALARM(STAT_INTERNAL_ERROR); TODO

  motors[motor].position = position;
}


int32_t motor_get_position(int motor) {return motors[motor].position;}


/// returns true if motor is in an error state
bool motor_error(int m) {
  uint8_t flags = motors[m].flags;
  if (calibrate_busy()) flags &= ~MOTOR_FLAG_STALLED_bm;
  return flags & MOTOR_FLAG_ERROR_bm;
}


bool motor_stalled(int m) {return motors[m].flags & MOTOR_FLAG_STALLED_bm;}
void motor_reset(int m) {motors[m].flags = 0;}


static void _set_power_state(int motor, motor_power_state_t state) {
  motors[motor].power_state = state;
  report_request();
}


static void _update_power(int motor) {
  motor_t *m = &motors[motor];

  switch (m->power_mode) {
  case MOTOR_POWERED_ONLY_WHEN_MOVING:
  case MOTOR_POWERED_IN_CYCLE:
    if (rtc_expired(m->power_timeout)) {
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


static void _load_move(int motor) {
  motor_t *m = &motors[motor];

  // Stop clock & DMA
  m->timer->CTRLA = 0;
  m->dma->CTRLA &= ~DMA_CH_ENABLE_bm;
  m->dma->CTRLB |= DMA_CH_TRNIF_bm | DMA_CH_ERRIF_bm; // Clear interrupt flags

  if (!m->steps) {
    m->active = false;
    return;
  }

  // Set direction
  if (m->clockwise) OUTCLR_PIN(m->dir_pin);
  else OUTSET_PIN(m->dir_pin);

  // Set power
  drv8711_set_power(motor, m->power);

  // Get clocks remaining in segment
  uint32_t clocks =
    (uint32_t)(SEGMENT_PERIOD - TIMER_STEP.CNT) * STEP_TIMER_DIV;

  // Count steps with phony DMA transfer
  m->dma->TRFCNT = m->steps;
  m->dma->CTRLA |= DMA_CH_ENABLE_bm;

  // Find the fastest clock rate that will fit the required number of steps
  uint32_t ticks_per_step = clocks / m->steps;
  uint8_t timer_clock;
  if (ticks_per_step <= 0xffff) timer_clock = TC_CLKSEL_DIV1_gc;
  else if (ticks_per_step <= 0x1ffff) timer_clock = TC_CLKSEL_DIV2_gc;
  else if (ticks_per_step <= 0x3ffff) timer_clock = TC_CLKSEL_DIV4_gc;
  else if (ticks_per_step <= 0x7ffff) timer_clock = TC_CLKSEL_DIV8_gc;
  else timer_clock = 0; // Clock off, too slow

  // Note, we rely on the fact that TC_CLKSEL_DIV1_gc through TC_CLKSEL_DIV8_gc
  // equal 1, 2, 3 & 4 respectively.
  uint16_t timer_period = ticks_per_step >> (timer_clock - 1);

  // Set clock and period
  if ((m->active = timer_period && timer_clock)) {
    m->timer->CNT = 0;
    m->timer->CCA = timer_period;     // Set step pulse period
    m->timer->CTRLA = timer_clock;    // Set clock rate
    m->steps = 0;
  }
}


ISR(M1_DMA_VECT) {_load_move(0);}
ISR(M2_DMA_VECT) {_load_move(1);}
ISR(M3_DMA_VECT) {_load_move(2);}
ISR(M4_DMA_VECT) {_load_move(3);}


void motor_load_move(int motor) {
  if (!motors[motor].active) _load_move(motor);
}


stat_t motor_prep_move(int motor, int32_t target) {
  motor_t *m = &motors[motor];

  // Validate input
  if (motor < 0 || MOTORS < motor) return ALARM(STAT_INTERNAL_ERROR);
  if (isinf(target)) return ALARM(STAT_MOVE_TARGET_INFINITE);
  if (isnan(target)) return ALARM(STAT_MOVE_TARGET_NAN);

  // Compute travel in steps
  int32_t steps = target - m->position;
  m->position = target;

  // Set direction, compensating for polarity
  bool negative = steps < 0;
  m->clockwise = !(negative ^ m->reverse);

  // Positive steps from here on
  if (negative) steps = -steps;
  m->steps = steps;

  // Compute power from axis velocity
  m->power = steps / (_get_max_velocity(motor) * SEGMENT_TIME);

  // Power motor
  switch (m->power_mode) {
  case MOTOR_POWERED_ONLY_WHEN_MOVING:
    if (!steps) break; // Not moving
    // Fall through

  case MOTOR_ALWAYS_POWERED: case MOTOR_POWERED_IN_CYCLE:
    // Reset timeout
    m->power_timeout = rtc_get_time() + MOTOR_IDLE_TIMEOUT * 1000;
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
int32_t get_encoder(int m) {return 0;}


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
