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

#include "plan/planner.h"
#include "plan/calibrate.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


typedef enum {
  MOTOR_IDLE,                   // motor stopped and may be partially energized
  MOTOR_ENERGIZING,
  MOTOR_ACTIVE
} motorPowerState_t;


typedef struct {
  // Config
  uint8_t motor_map;             // map motor to axis
  uint16_t microsteps;           // microsteps per full step
  cmMotorPolarity_t polarity;
  cmMotorPowerMode_t power_mode;
  float step_angle;              // degrees per whole step
  float travel_rev;              // mm or deg of travel per motor revolution
  PORT_t *port;
  TC0_t *timer;
  DMA_CH_t *dma;
  uint8_t dma_trigger;

  // Runtime state
  motorPowerState_t power_state; // state machine for managing motor power
  uint32_t timeout;
  cmMotorFlags_t flags;
  int32_t encoder;
  uint16_t steps;
  uint8_t last_clock;

  // Move prep
  uint8_t timer_clock;           // clock divisor setting or zero for off
  uint16_t timer_period;         // clock period counter
  bool positive;                 // step sign
  cmDirection_t direction;       // travel direction corrected for polarity

  // Step error correction
  int32_t correction_holdoff;    // count down segments between corrections
  float corrected_steps;         // accumulated for cycle (diagnostic)
} motor_t;


static motor_t motors[MOTORS] = {
  {
    .motor_map   = M1_MOTOR_MAP,
    .step_angle  = M1_STEP_ANGLE,
    .travel_rev  = M1_TRAVEL_PER_REV,
    .microsteps  = M1_MICROSTEPS,
    .polarity    = M1_POLARITY,
    .power_mode  = M1_POWER_MODE,
    .port        = &PORT_MOTOR_1,
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
    .port        = &PORT_MOTOR_2,
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
    .port        = &PORT_MOTOR_3,
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
    .port        = &PORT_MOTOR_4,
    .timer       = &M4_TIMER,
    .dma         = &M4_DMA_CH,
    .dma_trigger = M4_DMA_TRIGGER,
  }
};


static uint8_t _dummy;


/// Special interrupt for X-axis
ISR(TCE1_CCA_vect) {
  PORT_MOTOR_1.OUTTGL = STEP_BIT_bm;
  motors[0].steps++;
}


void motor_init() {
  // Reset position
  mp_set_steps_to_runtime_position();

  // Enable DMA
  DMA.CTRL = DMA_RESET_bm;
  DMA.CTRL = DMA_ENABLE_bm;
  DMA.INTFLAGS = 0xff; // clear all interrups

  for (int motor = 0; motor < MOTORS; motor++) {
    motor_t *m = &motors[motor];

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


void motor_shutdown(int motor) {
  motors[motor].port->OUTSET = MOTOR_ENABLE_BIT_bm; // Disable
}


int motor_get_axis(int motor) {
  return motors[motor].motor_map;
}


float motor_get_steps_per_unit(int motor) {
  return 360 * motors[motor].microsteps / motors[motor].travel_rev /
    motors[motor].step_angle;
}


int32_t motor_get_encoder(int motor) {
  return motors[motor].encoder;
}


void motor_set_encoder(int motor, float encoder) {
  motors[motor].encoder = round(encoder);
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
stat_t motor_power_callback() { // called by controller
  for (int motor = 0; motor < MOTORS; motor++)
    // Deenergize motor if disabled, in error or after timeout when not holding
    if (motors[motor].power_mode == MOTOR_DISABLED || motor_error(motor) ||
        motors[motor].timeout < rtc_get_time())
      _deenergize(motor);

  return STAT_OK;
}


void motor_error_callback(int motor, cmMotorFlags_t errors) {
  if (motors[motor].power_state != MOTOR_ACTIVE) return;

  motors[motor].flags |= errors;
  report_request();

  if (motor_error(motor)) {
    _deenergize(motor);

    // Stop and flush motion
    cm_request_feedhold();
    cm_request_queue_flush();
  }
}


void motor_prep_move(int motor, uint32_t seg_clocks, float travel_steps,
                     float error) {
  motor_t *m = &motors[motor];

#ifdef __STEP_CORRECTION
  // 'Nudge' correction strategy. Inject a single, scaled correction value
  // then hold off
  if (--m->correction_holdoff < 0 &&
      STEP_CORRECTION_THRESHOLD < fabs(error)) {

    m->correction_holdoff = STEP_CORRECTION_HOLDOFF;
    float correction = error * STEP_CORRECTION_FACTOR;

    if (0 < correction)
      correction = min3(correction, fabs(travel_steps), STEP_CORRECTION_MAX);
    else correction =
           max3(correction, -fabs(travel_steps), -STEP_CORRECTION_MAX);

    m->corrected_steps += correction;
    travel_steps -= correction;
  }
#endif

  // Power motor
  switch (motors[motor].power_mode) {
  case MOTOR_DISABLED: return;

  case MOTOR_POWERED_ONLY_WHEN_MOVING:
    if (fp_ZERO(travel_steps)) return; // Not moving
    // Fall through

  case MOTOR_ALWAYS_POWERED: case MOTOR_POWERED_IN_CYCLE:
    _energize(motor);
    break;

  case MOTOR_POWER_MODE_MAX_VALUE: break; // Shouldn't get here
  }

  // Compute motor timer clock and period. Rounding is performed to eliminate
  // a negative bias in the uint32_t conversion that results in long-term
  // negative drift.
  uint32_t ticks_per_step = round(fabs(seg_clocks / travel_steps));

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

  if (!ticks_per_step || fabs(travel_steps) < 0.001) m->timer_clock = 0;
  m->timer_period = ticks_per_step;
  m->positive = 0 <= travel_steps;

  // Setup the direction, compensating for polarity.
  if (m->positive) m->direction = DIRECTION_CW ^ m->polarity;
  else m->direction = DIRECTION_CCW ^ m->polarity;
}


void motor_load_move(int motor) {
  motor_t *m = &motors[motor];

  // Get actual step count from DMA channel
  uint16_t steps = 0xffff - m->dma->TRFCNT;
  m->dma->TRFCNT = 0xffff;
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
  if (m->direction == DIRECTION_CW) m->port->OUTCLR = DIRECTION_BIT_bm;
  else m->port->OUTSET = DIRECTION_BIT_bm;

  // Accumulate encoder
  if (!motor) {
    steps = m->steps;
    m->steps = 0;
  }
  m->encoder += m->positive ? steps : -(int32_t)steps;
}


void motor_end_move(int motor) {
  motors[motor].dma->CTRLA &= ~DMA_CH_ENABLE_bm;
  motors[motor].timer->CTRLA = 0; // Stop clock
}


// Var callbacks
float get_step_angle(int motor) {
  return motors[motor].step_angle;
}


void set_step_angle(int motor, float value) {
  motors[motor].step_angle = value;
}


float get_travel(int motor) {
  return motors[motor].travel_rev;
}


void set_travel(int motor, float value) {
  motors[motor].travel_rev = value;
}


uint16_t get_microstep(int motor) {
  return motors[motor].microsteps;
}


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


void set_polarity(int motor, uint8_t value) {
  motors[motor].polarity = value;
}


uint8_t get_motor_map(int motor) {
  return motors[motor].motor_map;
}


void set_motor_map(int motor, uint16_t value) {
  if (value < AXES) motors[motor].motor_map = value;
}


uint8_t get_power_mode(int motor) {
  return motors[motor].power_mode;
}


void set_power_mode(int motor, uint16_t value) {
  if (value < MOTOR_POWER_MODE_MAX_VALUE)
    motors[motor].power_mode = value;
}



uint8_t get_status_flags(int motor) {
  return motors[motor].flags;
}


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

  putchar('"');
}


uint8_t get_status_strings(int motor) {
  return get_status_flags(motor);
}


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
