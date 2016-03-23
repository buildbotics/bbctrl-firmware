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

#include <avr/interrupt.h>


// Compile-time settings
#define __STEP_CORRECTION
//#define __JERK_EXEC            // Use computed jerk (vs. forward difference)
//#define __KAHAN                // Use Kahan summation in aline exec functions
#define __CLOCK_EXTERNAL_16MHZ // uses PLL to provide 32 MHz system clock


#define AXES                     6 // number of axes
#define MOTORS                   4 // number of motors on the board
#define COORDS                   6 // number of supported coordinate systems
#define SWITCHES                 8 // number of supported limit switches
#define PWMS                     2 // number of supported PWM channels


// Axes
typedef enum {
  AXIS_X, AXIS_Y, AXIS_Z,
  AXIS_A, AXIS_B, AXIS_C,
  AXIS_U, AXIS_V, AXIS_W // reserved
} axis_t;


// Motor settings
#define MOTOR_CURRENT            0.8   // 1.0 is full power
#define MOTOR_IDLE_CURRENT       0.1   // 1.0 is full power
#define MOTOR_MICROSTEPS         16
#define MOTOR_POWER_MODE         MOTOR_POWERED_ONLY_WHEN_MOVING // See stepper.c
#define MOTOR_IDLE_TIMEOUT       2     // secs, motor off after this time

#define M1_MOTOR_MAP             AXIS_X
#define M1_STEP_ANGLE            1.8
#define M1_TRAVEL_PER_REV        3.175
#define M1_MICROSTEPS            MOTOR_MICROSTEPS
#define M1_POLARITY              MOTOR_POLARITY_NORMAL
#define M1_POWER_MODE            MOTOR_POWER_MODE

#define M2_MOTOR_MAP             AXIS_Y
#define M2_STEP_ANGLE            1.8
#define M2_TRAVEL_PER_REV        3.175
#define M2_MICROSTEPS            MOTOR_MICROSTEPS
#define M2_POLARITY              MOTOR_POLARITY_NORMAL
#define M2_POWER_MODE            MOTOR_POWER_MODE

#define M3_MOTOR_MAP             AXIS_A
#define M3_STEP_ANGLE            1.8
#define M3_TRAVEL_PER_REV        360 // degrees per motor rev
#define M3_MICROSTEPS            MOTOR_MICROSTEPS
#define M3_POLARITY              MOTOR_POLARITY_NORMAL
#define M3_POWER_MODE            MOTOR_POWER_MODE

#define M4_MOTOR_MAP             AXIS_Z
#define M4_STEP_ANGLE            1.8
#define M4_TRAVEL_PER_REV        3.175
#define M4_MICROSTEPS            MOTOR_MICROSTEPS
#define M4_POLARITY              MOTOR_POLARITY_NORMAL
#define M4_POWER_MODE            MOTOR_POWER_MODE


// Switch settings.  See switch.h
#define SWITCH_TYPE              SW_TYPE_NORMALLY_OPEN
#define X_SWITCH_MODE_MIN        SW_MODE_HOMING
#define X_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define Y_SWITCH_MODE_MIN        SW_MODE_HOMING
#define Y_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define Z_SWITCH_MODE_MIN        SW_MODE_DISABLED
#define Z_SWITCH_MODE_MAX        SW_MODE_HOMING
#define A_SWITCH_MODE_MIN        SW_MODE_HOMING
#define A_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define B_SWITCH_MODE_MIN        SW_MODE_HOMING
#define B_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define C_SWITCH_MODE_MIN        SW_MODE_HOMING
#define C_SWITCH_MODE_MAX        SW_MODE_DISABLED

// Switch ISRs
#define X_SWITCH_ISR_vect        PORTA_INT0_vect
#define Y_SWITCH_ISR_vect        PORTD_INT0_vect
#define Z_SWITCH_ISR_vect        PORTE_INT0_vect
#define A_SWITCH_ISR_vect        PORTF_INT0_vect
#define SWITCH_INTLVL            PORT_INT0LVL_MED_gc

// Timer for debouncing switches
#define SW_LOCKOUT_TICKS         250 // ms
#define SW_DEGLITCH_TICKS        30  // ms


// Machine settings
#define CHORDAL_TOLERANCE        0.01   // chordal accuracy for arc drawing
#define SOFT_LIMIT_ENABLE        0      // 0 = off, 1 = on
#define JERK_MAX                 50     // yes, that's km/min^3
#define JUNCTION_DEVIATION       0.05   // default value, in mm
#define JUNCTION_ACCELERATION    100000 // centripetal corner acceleration
#define JOG_ACCELERATION         500000 // mm/min^2

// Axis settings
#define VELOCITY_MAX             15000  // mm/min
#define FEEDRATE_MAX             VELOCITY_MAX

#define X_AXIS_MODE              AXIS_STANDARD // See canonical_machine.h
#define X_VELOCITY_MAX           VELOCITY_MAX  // G0 max velocity in mm/min
#define X_FEEDRATE_MAX           FEEDRATE_MAX  // G1 max feed rate in mm/min
#define X_TRAVEL_MIN             0             // minimum travel for soft limits
#define X_TRAVEL_MAX             150           // between switches or crashes
#define X_JERK_MAX               JERK_MAX
#define X_JERK_HOMING            (X_JERK_MAX * 2)
#define X_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define X_SEARCH_VELOCITY        500           // move in negative direction
#define X_LATCH_VELOCITY         100           // mm/min
#define X_LATCH_BACKOFF          5             // mm
#define X_ZERO_BACKOFF           1             // mm

#define Y_AXIS_MODE              AXIS_STANDARD
#define Y_VELOCITY_MAX           VELOCITY_MAX
#define Y_FEEDRATE_MAX           FEEDRATE_MAX
#define Y_TRAVEL_MIN             0
#define Y_TRAVEL_MAX             150
#define Y_JERK_MAX               JERK_MAX
#define Y_JERK_HOMING            (Y_JERK_MAX * 2)
#define Y_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define Y_SEARCH_VELOCITY        500
#define Y_LATCH_VELOCITY         100
#define Y_LATCH_BACKOFF          5
#define Y_ZERO_BACKOFF           1

#define Z_AXIS_MODE              AXIS_STANDARD
#define Z_VELOCITY_MAX           VELOCITY_MAX
#define Z_FEEDRATE_MAX           FEEDRATE_MAX
#define Z_TRAVEL_MIN             0
#define Z_TRAVEL_MAX             75
#define Z_JERK_MAX               JERK_MAX
#define Z_JERK_HOMING            (Z_JERK_MAX * 2)
#define Z_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define Z_SEARCH_VELOCITY        400
#define Z_LATCH_VELOCITY         100
#define Z_LATCH_BACKOFF          5
#define Z_ZERO_BACKOFF           1

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


// PWM settings
#define P1_PWM_FREQUENCY         100    // in Hz
#define P1_CW_SPEED_LO           1000   // in RPM (arbitrary units)
#define P1_CW_SPEED_HI           2000
#define P1_CW_PHASE_LO           0.125  // phase [0..1]
#define P1_CW_PHASE_HI           0.2
#define P1_CCW_SPEED_LO          1000
#define P1_CCW_SPEED_HI          2000
#define P1_CCW_PHASE_LO          0.125
#define P1_CCW_PHASE_HI          0.2
#define P1_PWM_PHASE_OFF         0.1


// Gcode defaults
#define GCODE_DEFAULT_UNITS         MILLIMETERS // MILLIMETERS or INCHES
#define GCODE_DEFAULT_PLANE         CANON_PLANE_XY // See canonical_machine.h
#define GCODE_DEFAULT_COORD_SYSTEM  G54 // G54, G55, G56, G57, G58 or G59
#define GCODE_DEFAULT_PATH_CONTROL  PATH_CONTINUOUS
#define GCODE_DEFAULT_DISTANCE_MODE ABSOLUTE_MODE


// Motors mapped to ports
#define PORT_MOTOR_1 PORTA
#define PORT_MOTOR_2 PORTF
#define PORT_MOTOR_3 PORTE
#define PORT_MOTOR_4 PORTD

// Motor fault ISRs
#define PORT_1_FAULT_ISR_vect PORTA_INT1_vect
#define PORT_2_FAULT_ISR_vect PORTD_INT1_vect
#define PORT_3_FAULT_ISR_vect PORTE_INT1_vect
#define PORT_4_FAULT_ISR_vect PORTF_INT1_vect

// Switch axes mapped to ports
#define PORT_SWITCH_X PORT_MOTOR_1
#define PORT_SWITCH_Y PORT_MOTOR_2
#define PORT_SWITCH_Z PORT_MOTOR_3
#define PORT_SWITCH_A PORT_MOTOR_4

// Axes mapped to output ports
#define PORT_OUT_X PORT_MOTOR_1
#define PORT_OUT_Y PORT_MOTOR_2
#define PORT_OUT_Z PORT_MOTOR_3
#define PORT_OUT_A PORT_MOTOR_4

#define MOTOR_PORT_DIR_gm 0x2f // pin dir settings

// Motor control port
#define STEP_BIT_bm         (1 << 0)
#define DIRECTION_BIT_bm    (1 << 1)
#define MOTOR_ENABLE_BIT_bm (1 << 2)
#define CHIP_SELECT_BIT_bm  (1 << 3)
#define FAULT_BIT_bm        (1 << 4)
#define GPIO1_OUT_BIT_bm    (1 << 5) // spindle and coolant
#define SW_MIN_BIT_bm       (1 << 6) // minimum switch inputs
#define SW_MAX_BIT_bm       (1 << 7) // maximum switch inputs

// Bit assignments for GPIO1_OUTs for spindle, PWM and coolant
#define SPINDLE_BIT         8 // spindle on/off
#define SPINDLE_DIR         4 // spindle direction, 1=CW, 0=CCW
#define SPINDLE_PWM         2 // spindle PWMs output bit
#define MIST_COOLANT_BIT    1 // coolant on/off (same as flood)
#define FLOOD_COOLANT_BIT   1 // coolant on/off (same as mist)

// Can use the spindle direction as an indicator LED.  See gpio.h
#define INDICATOR_LED       SPINDLE_DIR_LED

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
 *        in a permanent sleep_mode() call in usart_putc() (usart.c) as no
 *        interrupt can release the sleep mode.
 */

// Timer assignments - see specific modules for details
#define TIMER_STEP      TCC0 // Step timer    (see stepper.h)
#define TIMER_TMC2660   TCC1 // TMC2660 timer (see tmc2660.h)
#define TIMER_PWM1      TCD1 // PWM timer #1  (see pwm.c)
#define TIMER_PWM2      TCD1 // PWM timer #2  (see pwm.c)

#define M1_TIMER        TCE1
#define M2_TIMER        TCF0
#define M3_TIMER        TCE0
#define M4_TIMER        TCD0

#define M1_TIMER_CC     CCA
#define M2_TIMER_CC     CCA
#define M3_TIMER_CC     CCA
#define M4_TIMER_CC     CCA

// Timer setup for stepper and dwells
#define STEP_TIMER_DISABLE   0
#define STEP_TIMER_ENABLE    TC_CLKSEL_DIV4_gc
#define STEP_TIMER_DIV       4
#define STEP_TIMER_FREQ      (F_CPU / STEP_TIMER_DIV)
#define STEP_TIMER_POLL      (STEP_TIMER_FREQ * 0.001)
#define STEP_TIMER_WGMODE    TC_WGMODE_NORMAL_gc // count to TOP & rollover
#define STEP_TIMER_ISR       TCC0_OVF_vect
#define STEP_TIMER_INTLVL    TC_OVFINTLVL_HI_gc

/* Step correction settings
 *
 * Step correction settings determine how the encoder error is fed
 * back to correct position errors.  Since the following_error is
 * running 2 segments behind the current segment you have to be
 * careful not to overcompensate. The threshold determines if a
 * correction should be applied, and the factor is how much. The
 * holdoff is how many segments before applying another correction. If
 * threshold is too small and/or amount too large and/or holdoff is
 * too small you may get a runaway correction and error will grow
 * instead of shrink (or oscillate).
 */
/// magnitude of forwarding error (in steps)
#define STEP_CORRECTION_THRESHOLD 2.00
/// apply to step correction for a single segment
#define STEP_CORRECTION_FACTOR    0.25
/// max step correction allowed in a single segment
#define STEP_CORRECTION_MAX       0.60
/// minimum wait between error correction
#define STEP_CORRECTION_HOLDOFF   5
#define STEP_INITIAL_DIRECTION    DIRECTION_CW


// TMC2660 driver settings
#define TMC2660_SPI_PORT       PORTC
#define TMC2660_SPI_SS_PIN     4
#define TMC2660_SPI_SCK_PIN    5
#define TMC2660_SPI_MISO_PIN   6
#define TMC2660_SPI_MOSI_PIN   7
#define TMC2660_TIMER          TCC1
#define TMC2660_POLL_RATE      0.01  // sec.  Must be in (0, 1]
#define TMC2660_STABILIZE_TIME 0.01  // sec.  Must be at least 1ms


// PWM settings
#define PWM1_CTRLB             (3 | TC1_CCBEN_bm) // single slope PWM channel B
#define PWM1_ISR_vect          TCD1_CCB_vect
#define PWM2_CTRLA_CLKSEL      TC_CLKSEL_DIV1_gc
#define PWM2_CTRLB             3                  // single slope PWM no output
#define PWM2_ISR_vect          TCE1_CCB_vect


// Input
#define INPUT_BUFFER_LEN         255 // text buffer size (255 max)


// Arc
#define ARC_RADIUS_ERROR_MAX 1.0   // max mm diff between start and end radius
#define ARC_RADIUS_ERROR_MIN 0.005 // min mm where 1% rule applies
#define ARC_RADIUS_TOLERANCE 0.001 // 0.1% radius variance test
