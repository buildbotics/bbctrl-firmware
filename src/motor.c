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

#include "motor.h"
#include "config.h"
#include "hardware.h"
#include "cpp_magic.h"
#include "rtc.h"
#include "report.h"
#include "stepper.h"
#include "tmc2660.h"
#include "estop.h"
#include "gcode_state.h"
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
  MOTOR_IDLE,                    // motor stopped and may be partially energized
  MOTOR_ENERGIZING,
  MOTOR_ACTIVE
} motor_power_state_t;


typedef struct {
  // Config
  uint8_t motor_map;             // map motor to axis
  uint16_t microsteps;           // microsteps per full step
  motor_polarity_t polarity;
  motor_power_mode_t power_mode;
  float step_angle;              // degrees per whole step
  float travel_rev;              // mm or deg of travel per motor revolution
  uint8_t step_pin;
  uint8_t dir_pin;
  uint8_t enable_pin;
  TC0_t *timer;
  DMA_CH_t *dma;
  uint8_t dma_trigger;

  // Runtime state
  motor_power_state_t power_state; // state machine for managing motor power
  uint32_t timeout;
  motor_flags_t flags;
  int32_t encoder;
  int32_t commanded;
  uint16_t steps;                // Currently used by the x-axis only
  uint8_t last_clock;
  bool wasEnabled;

  // Move prep
  uint8_t timer_clock;           // clock divisor setting or zero for off
  uint16_t timer_period;         // clock period counter
  bool positive;                 // step sign
  direction_t direction;         // travel direction corrected for polarity
  int32_t position;
  int32_t error;
} motor_t;


static motor_t motors[MOTORS] = {
  {
    .motor_map   = M1_MOTOR_MAP,
    .step_angle  = M1_STEP_ANGLE,
    .travel_rev  = M1_TRAVEL_PER_REV,
    .microsteps  = M1_MICROSTEPS,
    .polarity    = M1_POLARITY,
    .power_mode  = M1_POWER_MODE,
    .step_pin    = STEP_X_PIN,
    .dir_pin     = DIR_X_PIN,
    .enable_pin  = ENABLE_X_PIN,
    .timer       = (TC0_t *)&M1_TIMER,
    .dma         = &M1_DMA_CH,
    .dma_trigger = M1_DMA_TRIGGER,
  }, {
    .motor_map   = M2_MOTOR_MAP,
    .step_angle  = M2_STEP_ANGLE,
    .travel_rev  = M2_TRAVEL_PER_REV,
    .microsteps  = M2_MICROSTEPS,
    .polarity    = M2_POLARITY,
    .power_mode  = M2_POWER_MODE,
    .step_pin    = STEP_Y_PIN,
    .dir_pin     = DIR_Y_PIN,
    .enable_pin  = ENABLE_Y_PIN,
    .timer       = &M2_TIMER,
    .dma         = &M2_DMA_CH,
    .dma_trigger = M2_DMA_TRIGGER,
  }, {
    .motor_map   = M3_MOTOR_MAP,
    .step_angle  = M3_STEP_ANGLE,
    .travel_rev  = M3_TRAVEL_PER_REV,
    .microsteps  = M3_MICROSTEPS,
    .polarity    = M3_POLARITY,
    .power_mode  = M3_POWER_MODE,
    .step_pin    = STEP_Z_PIN,
    .dir_pin     = DIR_Z_PIN,
    .enable_pin  = ENABLE_Z_PIN,
    .timer       = &M3_TIMER,
    .dma         = &M3_DMA_CH,
    .dma_trigger = M3_DMA_TRIGGER,
  }, {
    .motor_map   = M4_MOTOR_MAP,
    .step_angle  = M4_STEP_ANGLE,
    .travel_rev  = M4_TRAVEL_PER_REV,
    .microsteps  = M4_MICROSTEPS,
    .polarity    = M4_POLARITY,
    .power_mode  = M4_POWER_MODE,
    .step_pin    = STEP_A_PIN,
    .dir_pin     = DIR_A_PIN,
    .enable_pin  = ENABLE_A_PIN,
    .timer       = &M4_TIMER,
    .dma         = &M4_DMA_CH,
    .dma_trigger = M4_DMA_TRIGGER,
  }
};


static uint8_t _dummy;


/// Special interrupt for X-axis
ISR(TCE1_CCA_vect) {
  OUTTGL_PIN(STEP_X_PIN);
  motors[0].steps++;
}


void motor_init() {
  // Enable DMA
  DMA.CTRL = DMA_RESET_bm;
  DMA.CTRL = DMA_ENABLE_bm;
  DMA.INTFLAGS = 0xff; // clear all interrupts

  for (int motor = 0; motor < MOTORS; motor++) {
    motor_t *m = &motors[motor];

    // IO pins
    DIRSET_PIN(m->step_pin);   // Output
    DIRSET_PIN(m->dir_pin);    // Output
    OUTSET_PIN(m->enable_pin); // High (disabled)
    DIRSET_PIN(m->enable_pin); // Output

    // Setup motor timer
    m->timer->CTRLB = TC_WGMODE_FRQ_gc | TC1_CCAEN_bm;

    if (!motor) continue; // Don't configure DMA for motor 0

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
  }

  // Setup special interrupt for X-axis mapping
  M1_TIMER.INTCTRLB = TC_CCAINTLVL_HI_gc;
}


void motor_enable(int motor, bool enable) {
  if (enable) OUTCLR_PIN(motors[motor].enable_pin); // Active low
  else {
    OUTSET_PIN(motors[motor].enable_pin);
    motors[motor].power_state = MOTOR_IDLE;
  }
}


int motor_get_axis(int motor) {return motors[motor].motor_map;}


float motor_get_steps_per_unit(int motor) {
  return 360 * motors[motor].microsteps / motors[motor].travel_rev /
    motors[motor].step_angle;
}


int32_t motor_get_encoder(int motor) {
  return motors[motor].encoder;
}


void motor_set_encoder(int motor, float encoder) {
  motor_t *m = &motors[motor];
  m->encoder = m->position = m->commanded = round(encoder);
  m->error = 0;
}


/// returns true if motor is in an error state
bool motor_error(int motor) {
  uint8_t flags = motors[motor].flags;
  if (calibrate_busy()) flags &= ~MOTOR_FLAG_STALLED_bm;
  return flags & MOTOR_FLAG_ERROR_bm;
}


bool motor_stalled(int motor) {
  return motors[motor].flags & MOTOR_FLAG_STALLED_bm;
}


void motor_reset(int motor) {
  motors[motor].flags = 0;
  tmc2660_reset(motor);
}


/// Remove power from a motor
static void _deenergize(int motor) {
  if (motors[motor].power_state == MOTOR_ACTIVE) {
    motors[motor].power_state = MOTOR_IDLE;
    tmc2660_disable(motor);
  }
}


/// Apply power to a motor
static void _energize(int motor) {
  if (motors[motor].power_state == MOTOR_IDLE && !motor_error(motor)) {
    motors[motor].power_state = MOTOR_ENERGIZING;
    tmc2660_enable(motor);
  }

  // Reset timeout, regardless
  motors[motor].timeout = rtc_get_time() + MOTOR_IDLE_TIMEOUT * 1000;
}


bool motor_energizing() {
  for (int motor = 0; motor < MOTORS; motor++)
    if (motors[motor].power_state == MOTOR_ENERGIZING)
      return true;

  return false;
}


void motor_driver_callback(int motor) {
  motor_t *m = &motors[motor];

  if (m->power_state == MOTOR_IDLE) m->flags &= ~MOTOR_FLAG_ENABLED_bm;
  else {
    m->power_state = MOTOR_ACTIVE;
    m->flags |= MOTOR_FLAG_ENABLED_bm;
  }

  report_request();
}


/// Callback to manage motor power sequencing and power-down timing.
stat_t motor_rtc_callback() { // called by controller
  for (int motor = 0; motor < MOTORS; motor++)
    // Deenergize motor if disabled or timedout
    if (motors[motor].power_mode == MOTOR_DISABLED ||
        rtc_expired(motors[motor].timeout)) _deenergize(motor);

  return STAT_OK;
}


void motor_error_callback(int motor, motor_flags_t errors) {
  if (motors[motor].power_state != MOTOR_ACTIVE) return;

  motors[motor].flags |= errors;
  report_request();

  if (motor_error(motor)) ALARM(STAT_MOTOR_ERROR);
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
  // TODO we currently accumulate the x-axis here
  if (!motor) {
    steps = m->steps;
    m->steps = 0;
  }
  if (!m->wasEnabled) steps = 0;
  m->encoder += m->positive ? steps : -(int32_t)steps;

  // Compute error
  m->error = m->commanded - m->encoder;
  m->commanded = m->position;
}


void motor_end_move(int motor) {
  motor_t *m = &motors[motor];
  m->wasEnabled = m->dma->CTRLA & DMA_CH_ENABLE_bm;
  m->dma->CTRLA &= ~DMA_CH_ENABLE_bm;
  m->timer->CTRLA = 0; // Stop clock
}


void motor_prep_move(int motor, int32_t seg_clocks, float target) {
  motor_t *m = &motors[motor];

  int32_t travel = round(target) - m->position + m->error;
  m->error = 0;

  // Power motor
  switch (m->power_mode) {
  case MOTOR_DISABLED: return;

  case MOTOR_POWERED_ONLY_WHEN_MOVING:
    if (!travel) return; // Not moving
    // Fall through

  case MOTOR_ALWAYS_POWERED: case MOTOR_POWERED_IN_CYCLE:
    _energize(motor); // TODO is ~5ms enough time to enable the motor?
    break;

  case MOTOR_POWER_MODE_MAX_VALUE: break; // Shouldn't get here
  }

  // Compute motor timer clock and period. Rounding is performed to eliminate
  // a negative bias in the uint32_t conversion that results in long-term
  // negative drift.
  int32_t ticks_per_step = labs(seg_clocks / travel);

  // Find the clock rate that will fit the required number of steps
  if (ticks_per_step & 0xffff0000UL) {
    ticks_per_step /= 2;

    if (ticks_per_step & 0xffff0000UL) {
      ticks_per_step /= 2;

      if (ticks_per_step & 0xffff0000UL) {
        ticks_per_step /= 2;

        if (ticks_per_step & 0xffff0000UL) m->timer_clock = 0; // Off, too slow
        else m->timer_clock = TC_CLKSEL_DIV8_gc;
      } else m->timer_clock = TC_CLKSEL_DIV4_gc;
    } else m->timer_clock = TC_CLKSEL_DIV2_gc;
  } else m->timer_clock = TC_CLKSEL_DIV1_gc;

  if (!ticks_per_step) m->timer_clock = 0;
  m->timer_period = ticks_per_step;
  m->positive = 0 <= travel;

  // Setup the direction, compensating for polarity.
  if (m->positive) m->direction = DIRECTION_CW ^ m->polarity;
  else m->direction = DIRECTION_CCW ^ m->polarity;

  // Compute actual steps
  if (m->timer_clock) {
    int32_t clocks = seg_clocks >> (m->timer_clock - 1); // Motor timer clocks
    int32_t steps = clocks / m->timer_period;

    // Update position
    m->position += m->positive ? steps : -steps;

    // Sanity check
    int32_t diff = labs(labs(travel) - steps);
    if (16 < diff) ALARM(STAT_STEP_CHECK_FAILED);
  }
}


// Var callbacks
float get_step_angle(int motor) {return motors[motor].step_angle;}
void set_step_angle(int motor, float value) {motors[motor].step_angle = value;}
float get_travel(int motor) {return motors[motor].travel_rev;}
void set_travel(int motor, float value) {motors[motor].travel_rev = value;}
uint16_t get_microstep(int motor) {return motors[motor].microsteps;}


void set_microstep(int motor, uint16_t value) {
  switch (value) {
  case 1: case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
    break;
  default: return;
  }

  motors[motor].microsteps = value;
}


uint8_t get_polarity(int motor) {
  if (motor < 0 || MOTORS <= motor) return 0;
  return motors[motor].polarity;
}


void set_polarity(int motor, uint8_t value) {motors[motor].polarity = value;}
uint8_t get_motor_map(int motor) {return motors[motor].motor_map;}


void set_motor_map(int motor, uint16_t value) {
  if (value < AXES) motors[motor].motor_map = value;
}


uint8_t get_power_mode(int motor) {return motors[motor].power_mode;}


void set_power_mode(int motor, uint16_t value) {
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

  if (MOTOR_FLAG_OVERTEMP_WARN_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("temp warn"));
    first = false;
  }

  if (MOTOR_FLAG_OVERTEMP_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("over temp"));
    first = false;
  }

  if (MOTOR_FLAG_SHORTED_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("short"));
    first = false;
  }

  if (MOTOR_FLAG_OPEN_LOAD_bm & flags) {
    if (!first) printf_P(PSTR(", "));
    printf_P(PSTR("open"));
    first = false;
  }

  putchar('"');
}


uint8_t get_status_strings(int motor) {return get_status_flags(motor);}


// Command callback
void command_mreset(int argc, char *argv[]) {
  if (argc == 1)
    for (int motor = 0; motor < MOTORS; motor++)
      motor_reset(motor);

  else {
    int motor = atoi(argv[1]);
    if (motor < MOTORS) motor_reset(motor);
  }

  report_request();
}
