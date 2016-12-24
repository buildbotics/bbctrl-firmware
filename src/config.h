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

#pragma once

#include "pins.h"

#include <avr/interrupt.h>


#define VERSION "0.3.0"


// Pins
enum {
  STALL_X_PIN = PORT_A << 3,
  STALL_Y_PIN,
  STALL_Z_PIN,
  STALL_A_PIN,
  SPIN_DIR_PIN,
  SPIN_ENABLE_PIN,
  ANALOG_PIN,
  PROBE_PIN,

  MIN_X_PIN = PORT_B << 3,
  MAX_X_PIN,
  MIN_A_PIN,
  MAX_A_PIN,
  MIN_Y_PIN,
  MAX_Y_PIN,
  MIN_Z_PIN,
  MAX_Z_PIN,

  SDA_PIN = PORT_C << 3,
  SCL_PIN,
  SERIAL_RX_PIN,
  SERIAL_TX_PIN,
  SERIAL_CTS_PIN,
  SPI_CLK_PIN,
  SPI_MISO_PIN,
  SPI_MOSI_PIN,

  STEP_X_PIN = PORT_D << 3,
  SPI_CS_X_PIN,
  SPI_CS_A_PIN,
  SPI_CS_Z_PIN,
  SPIN_PWM_PIN,
  SWITCH_1_PIN,
  RS485_RO_PIN,
  RS485_DI_PIN,

  STEP_Y_PIN = PORT_E << 3,
  SPI_CS_Y_PIN,
  DIR_X_PIN,
  DIR_Y_PIN,
  STEP_A_PIN,
  SWITCH_2_PIN,
  DIR_Z_PIN,
  DIR_A_PIN,

  STEP_Z_PIN = PORT_F << 3,
  RS485_RW_PIN,
  FAULT_PIN,
  ESTOP_PIN,
  MOTOR_FAULT_PIN,
  MOTOR_ENABLE_PIN,
  NC_0_PIN,
  NC_1_PIN,
};

#define SPI_SS_PIN SERIAL_CTS_PIN // Needed for SPI configuration


// Compile-time settings
#define __CLOCK_EXTERNAL_16MHZ   // uses PLL to provide 32 MHz system clock
//#define __CLOCK_INTERNAL_32MHZ


#define AXES                     6 // number of axes
#define MOTORS                   4 // number of motors on the board
#define COORDS                   6 // number of supported coordinate systems
#define SWITCHES                 9 // number of supported limit switches
#define PWMS                     2 // number of supported PWM channels


// Motor settings.  See motor.c
#define MOTOR_MAX_CURRENT        1.0  // 1.0 is full power
#define MOTOR_MIN_CURRENT        0.15 // 1.0 is full power
#define MOTOR_IDLE_CURRENT       0.05 // 1.0 is full power
#define MOTOR_STALL_THRESHOLD    0    // 0 -> 1 is least -> most sensitive
#define MOTOR_MICROSTEPS         32
#define MOTOR_POWER_MODE         MOTOR_POWERED_ONLY_WHEN_MOVING
#define MOTOR_IDLE_TIMEOUT       0.25  // secs, motor off after this time

#define M1_MOTOR_MAP             AXIS_X
#define M1_STEP_ANGLE            1.8
#define M1_TRAVEL_PER_REV        6.35
#define M1_MICROSTEPS            MOTOR_MICROSTEPS
#define M1_POLARITY              MOTOR_POLARITY_NORMAL
#define M1_POWER_MODE            MOTOR_POWER_MODE

#define M2_MOTOR_MAP             AXIS_Y
#define M2_STEP_ANGLE            1.8
#define M2_TRAVEL_PER_REV        6.35
#define M2_MICROSTEPS            MOTOR_MICROSTEPS
#define M2_POLARITY              MOTOR_POLARITY_NORMAL
#define M2_POWER_MODE            MOTOR_POWER_MODE

#define M3_MOTOR_MAP             AXIS_Z
#define M3_STEP_ANGLE            1.8
#define M3_TRAVEL_PER_REV        (25.4 / 6.0)
#define M3_MICROSTEPS            MOTOR_MICROSTEPS
#define M3_POLARITY              MOTOR_POLARITY_NORMAL
#define M3_POWER_MODE            MOTOR_POWER_MODE

#define M4_MOTOR_MAP             AXIS_A
#define M4_STEP_ANGLE            1.8
#define M4_TRAVEL_PER_REV        360 // degrees per motor rev
#define M4_MICROSTEPS            MOTOR_MICROSTEPS
#define M4_POLARITY              MOTOR_POLARITY_NORMAL
#define M4_POWER_MODE            MOTOR_POWER_MODE


// Switch settings.  See switch.c
#define SWITCH_INTLVL            PORT_INT0LVL_MED_gc
#define SW_LOCKOUT_TICKS         250 // ms
#define SW_DEGLITCH_TICKS        30  // ms


// Machine settings
//#define STEP_CORRECTION                        // Enable step correction
#define MAX_STEP_CORRECTION      4             // In steps per segment
#define CHORDAL_TOLERANCE        0.01          // chordal accuracy for arcs
#define JERK_MAX                 50            // yes, that's km/min^3
#define JUNCTION_DEVIATION       0.05          // default value, in mm
#define JUNCTION_ACCELERATION    100000        // centripetal corner accel
#define JOG_JERK_MULT            4             // Jogging jerk multipler
#define JOG_MIN_VELOCITY         10            // mm/min
#define CAL_ACCELERATION         500000        // mm/min^2

// Axis settings
#define VELOCITY_MAX             10000         // mm/min
#define FEEDRATE_MAX             VELOCITY_MAX

#define X_AXIS_MODE              AXIS_STANDARD // See machine.h
#define X_VELOCITY_MAX           VELOCITY_MAX  // G0 max velocity in mm/min
#define X_FEEDRATE_MAX           FEEDRATE_MAX  // G1 max feed rate in mm/min
#define X_TRAVEL_MIN             0             // minimum travel for soft limits
#define X_TRAVEL_MAX             300           // between switches or crashes
#define X_JERK_MAX               JERK_MAX
#define X_JERK_HOMING            (X_JERK_MAX * 2)
#define X_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define X_SEARCH_VELOCITY        3000          // move in negative direction
#define X_LATCH_VELOCITY         100           // mm/min
#define X_LATCH_BACKOFF          5             // mm
#define X_ZERO_BACKOFF           1             // mm
#define X_HOMING_MODE            HOMING_STALL_MIN

#define Y_AXIS_MODE              AXIS_STANDARD
#define Y_VELOCITY_MAX           VELOCITY_MAX
#define Y_FEEDRATE_MAX           FEEDRATE_MAX
#define Y_TRAVEL_MIN             0
#define Y_TRAVEL_MAX             300
#define Y_JERK_MAX               JERK_MAX
#define Y_JERK_HOMING            (Y_JERK_MAX * 2)
#define Y_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define Y_SEARCH_VELOCITY        3000
#define Y_LATCH_VELOCITY         100
#define Y_LATCH_BACKOFF          5
#define Y_ZERO_BACKOFF           1
#define Y_HOMING_MODE            HOMING_STALL_MIN

#define Z_AXIS_MODE              AXIS_STANDARD
#define Z_VELOCITY_MAX           2000 // VELOCITY_MAX
#define Z_FEEDRATE_MAX           FEEDRATE_MAX
#define Z_TRAVEL_MIN             0
#define Z_TRAVEL_MAX             75
#define Z_JERK_MAX               JERK_MAX
#define Z_JERK_HOMING            (Z_JERK_MAX * 2)
#define Z_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define Z_SEARCH_VELOCITY        800
#define Z_LATCH_VELOCITY         100
#define Z_LATCH_BACKOFF          5
#define Z_ZERO_BACKOFF           1
#define Z_HOMING_MODE            HOMING_DISABLED //STALL_MAX

// A values are chosen to make the A motor react the same as X for testing
// set to the same speed as X axis
#define A_AXIS_MODE              AXIS_RADIUS
#define A_VELOCITY_MAX           (X_VELOCITY_MAX / M1_TRAVEL_PER_REV * 360)
#define A_FEEDRATE_MAX           A_VELOCITY_MAX
#define A_TRAVEL_MIN             -1
#define A_TRAVEL_MAX             -1 // same value means infinite
#define A_JERK_MAX               (X_JERK_MAX * 360 / M1_TRAVEL_PER_REV)
#define A_JERK_HOMING            (A_JERK_MAX * 2)
#define A_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define A_RADIUS                 (M1_TRAVEL_PER_REV / 2 / M_PI)
#define A_SEARCH_VELOCITY        600
#define A_LATCH_VELOCITY         100
#define A_LATCH_BACKOFF          5
#define A_ZERO_BACKOFF           2
#define A_HOMING_MODE            HOMING_DISABLED

#define B_AXIS_MODE              AXIS_DISABLED
#define B_VELOCITY_MAX           3600
#define B_FEEDRATE_MAX           B_VELOCITY_MAX
#define B_TRAVEL_MIN             -1
#define B_TRAVEL_MAX             -1
#define B_JERK_MAX               JERK_MAX
#define B_JERK_HOMING            (B_JERK_MAX * 2)
#define B_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define B_RADIUS                 1
#define B_SEARCH_VELOCITY        600
#define B_LATCH_VELOCITY         100
#define B_LATCH_BACKOFF          5
#define B_ZERO_BACKOFF           2

#define C_AXIS_MODE              AXIS_DISABLED
#define C_VELOCITY_MAX           3600
#define C_FEEDRATE_MAX           C_VELOCITY_MAX
#define C_TRAVEL_MIN             -1
#define C_TRAVEL_MAX             -1
#define C_JERK_MAX               JERK_MAX
#define C_JERK_HOMING            (C_JERK_MAX * 2)
#define C_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define C_RADIUS                 1
#define C_SEARCH_VELOCITY        600
#define C_LATCH_VELOCITY         100
#define C_LATCH_BACKOFF          5
#define C_ZERO_BACKOFF           2


// Spindle settings
#define SPINDLE_TYPE             SPINDLE_TYPE_HUANYANG
#define SPINDLE_PWM_FREQUENCY    100    // in Hz
#define SPINDLE_MIN_RPM          1000
#define SPINDLE_MAX_RPM          24000
#define SPINDLE_MIN_DUTY         0.05
#define SPINDLE_MAX_DUTY         0.99
#define SPINDLE_POLARITY         0 // 0 = normal, 1 = reverse


// Gcode defaults
#define GCODE_DEFAULT_UNITS         MILLIMETERS // MILLIMETERS or INCHES
#define GCODE_DEFAULT_PLANE         PLANE_XY    // See machine.h
#define GCODE_DEFAULT_COORD_SYSTEM  G54         // G54, G55, G56, G57, G58, G59
#define GCODE_DEFAULT_PATH_CONTROL  PATH_CONTINUOUS
#define GCODE_DEFAULT_DISTANCE_MODE ABSOLUTE_MODE
#define GCODE_DEFAULT_ARC_DISTANCE_MODE INCREMENTAL_MODE


// Motor ISRs
#define STALL_ISR_vect PORTA_INT1_vect
#define FAULT_ISR_vect PORTF_INT1_vect


/* Interrupt usage:
 *
 *    HI    Stepper timers                       (set in stepper.h)
 *    LO    Segment execution SW interrupt       (set in stepper.h)
 *   MED    GPIO1 switch port                    (set in gpio.h)
 *   MED    Serial RX                            (set in usart.c)
 *   MED    Serial TX                            (set in usart.c) (* see note)
 *    LO    Real time clock interrupt            (set in xmega_rtc.h)
 *
 *    (*) The TX cannot run at LO level or exception reports and other prints
 *        called from a LO interrupt (as in prep_line()) will kill the system
 *        in a permanent loop call in usart_putc() (usart.c).
 */

// Timer assignments - see specific modules for details
// TCC1 free
#define TIMER_STEP             TCC0 // Step timer    (see stepper.h)
#define TIMER_PWM              TCD1 // PWM timer     (see pwm_spindle.c)

#define M1_TIMER               TCD0
#define M2_TIMER               TCE0
#define M3_TIMER               TCF0
#define M4_TIMER               TCE1

#define M1_DMA_CH              DMA.CH0
#define M2_DMA_CH              DMA.CH1
#define M3_DMA_CH              DMA.CH2
#define M4_DMA_CH              DMA.CH3

#define M1_DMA_TRIGGER         DMA_CH_TRIGSRC_TCD0_CCA_gc
#define M2_DMA_TRIGGER         DMA_CH_TRIGSRC_TCE0_CCA_gc
#define M3_DMA_TRIGGER         DMA_CH_TRIGSRC_TCF0_CCA_gc
#define M4_DMA_TRIGGER         DMA_CH_TRIGSRC_TCE1_CCA_gc


// Timer setup for stepper and dwells
#define STEP_TIMER_DISABLE     0
#define STEP_TIMER_ENABLE      TC_CLKSEL_DIV4_gc
#define STEP_TIMER_DIV         4
#define STEP_TIMER_FREQ        (F_CPU / STEP_TIMER_DIV)
#define STEP_TIMER_POLL        (STEP_TIMER_FREQ * 0.001)
#define STEP_TIMER_WGMODE      TC_WGMODE_NORMAL_gc // count to TOP & rollover
#define STEP_TIMER_ISR         TCC0_OVF_vect
#define STEP_TIMER_INTLVL      TC_OVFINTLVL_HI_gc

#define MAX_SEGMENT_TIME       ((float)0xffff / 60.0 / STEP_TIMER_FREQ)
#define NOM_SEGMENT_USEC       5000.0 // nominal segment time
#define MIN_SEGMENT_USEC       2500.0 // minimum segment time


// Huanyang settings
#define HUANYANG_PORT          USARTD1
#define HUANYANG_DRE_vect      USARTD1_DRE_vect
#define HUANYANG_TXC_vect      USARTD1_TXC_vect
#define HUANYANG_RXC_vect      USARTD1_RXC_vect
#define HUANYANG_TIMEOUT       50 // ms. response timeout
#define HUANYANG_RETRIES        4 // Number of retries before failure
#define HUANYANG_ID             1 // Default ID


// Serial settings
#define SERIAL_BAUD            USART_BAUD_115200
#define SERIAL_PORT            USARTC0
#define SERIAL_DRE_vect        USARTC0_DRE_vect
#define SERIAL_RXC_vect        USARTC0_RXC_vect


// Input
#define INPUT_BUFFER_LEN       255 // text buffer size (255 max)


// Arc
#define ARC_RADIUS_ERROR_MAX   1.0   // max mm diff between start and end radius
#define ARC_RADIUS_ERROR_MIN   0.005 // min mm where 1% rule applies
#define ARC_RADIUS_TOLERANCE   0.001 // 0.1% radius variance test


// Planner
/// Should be at least the number of buffers required to support optimal
/// planning in the case of very short lines or arc segments.  Suggest no less
/// than 12.  Maximum is 255 with out also changing the type of mb.space.  Must
/// leave headroom for stack.
#define PLANNER_BUFFER_POOL_SIZE 32

/// Buffers to reserve in planner before processing new input line
#define PLANNER_BUFFER_HEADROOM   4

/// Minimum number of filled buffers before timeout until execution starts
#define PLANNER_EXEC_MIN_FILL     4

/// Delay before executing new buffers unless PLANNER_EXEC_MIN_FILL is met
/// This gives the planner a chance to make a good plan before execution starts
#define PLANNER_EXEC_DELAY      250 // In ms


// I2C
#define I2C_DEV                 TWIC
#define I2C_ISR                 TWIC_TWIS_vect
#define I2C_ADDR                0x2b
#define I2C_MAX_DATA            8
