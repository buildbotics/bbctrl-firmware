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

#pragma once

#include "pins.h"

#ifdef __AVR__
#include <avr/interrupt.h>
#endif


// Pins
enum {
  STALL_X_PIN = PIN_ID(PORT_A, 0),
  STALL_Y_PIN,
  STALL_Z_PIN,
  STALL_A_PIN,
  TOOL_DIR_PIN,
  TOOL_ENABLE_PIN,
  ANALOG_1_PIN,
  ANALOG_2_PIN,

  MIN_X_PIN = PIN_ID(PORT_B, 0),
  MAX_X_PIN,
  MIN_A_PIN,
  MAX_A_PIN,
  MIN_Y_PIN,
  MAX_Y_PIN,
  MIN_Z_PIN,
  MAX_Z_PIN,

  SDA_PIN = PIN_ID(PORT_C, 0),
  SCL_PIN,
  SERIAL_RX_PIN,
  SERIAL_TX_PIN,
  SERIAL_CTS_PIN,
  SPI_CLK_PIN,
  SPI_MISO_PIN,
  SPI_MOSI_PIN,

  STEP_X_PIN = PIN_ID(PORT_D, 0),
  SPI_CS_X_PIN,
  SPI_CS_A_PIN,
  SPI_CS_Z_PIN,
  PWM_PIN,
  SWITCH_2_PIN,
  RS485_RO_PIN,
  RS485_DI_PIN,

  STEP_Y_PIN = PIN_ID(PORT_E, 0),
  SPI_CS_Y_PIN,
  DIR_X_PIN,
  DIR_Y_PIN,
  STEP_A_PIN,
  SWITCH_1_PIN,
  DIR_Z_PIN,
  DIR_A_PIN,

  STEP_Z_PIN = PIN_ID(PORT_F, 0),
  RS485_RW_PIN,
  FAULT_PIN,
  ESTOP_PIN,
  MOTOR_FAULT_PIN,
  MOTOR_ENABLE_PIN,
  TEST_PIN,
  PROBE_PIN,
};

#define SPI_SS_PIN               SERIAL_CTS_PIN // Needed for SPI configuration


#define AXES                     6 // number of axes
#define MOTORS                   4 // number of motors on the board
#define OUTS                     6 // number of supported pin outputs
#define ANALOG                   2 // number of supported analog inputs
#define VFDREG                  32 // number of supported VFD modbus registers

// Switch settings.  See switch.c
#define SWITCH_DEBOUNCE          5 // ms


// Motor ISRs
#define STALL_ISR_vect           PORTA_INT1_vect
#define FAULT_ISR_vect           PORTF_INT1_vect


/* Interrupt usage:
 *
 *    HI    Stepper timers                       (set in stepper.h)
 *    LO    Segment execution SW interrupt       (set in stepper.h)
 *   MED    Serial RX                            (set in usart.c)
 *   MED    Serial TX                            (set in usart.c) (* see note)
 *   MED    I2C Slave                            (set in i2c.c)
 *    LO    Real time clock interrupt            (set in rtc.h)
 *
 *    (*) The TX cannot run at LO level or exception reports and other prints
 *        called from a LO interrupt (as in prep_line()) will kill the system
 *        in a permanent loop call in usart_putc() (usart.c).
 */

// Timer assignments
// NOTE, TCC1 free
#define TIMER_STEP               TCC0 // Step timer (see stepper.h)
#define TIMER_PWM                TCD1 // PWM timer  (see pwm.c)


// Timer setup for stepper and dwells
#define STEP_TIMER_ENABLE        TC_CLKSEL_DIV8_gc
#define STEP_TIMER_DIV           8
#define STEP_TIMER_FREQ          (F_CPU / STEP_TIMER_DIV)
#define STEP_TIMER_POLL          ((uint16_t)(STEP_TIMER_FREQ * 0.001)) // 1ms
#define STEP_TIMER_WGMODE        TC_WGMODE_NORMAL_gc // count to TOP & rollover
#define STEP_TIMER_ISR           TCC0_OVF_vect
#define STEP_TIMER_INTLVL        TC_OVFINTLVL_HI_gc
#define STEP_LOW_LEVEL_ISR       ADCB_CH0_vect
#define STEP_PULSE_WIDTH         (F_CPU * 0.000002 / 2) // 2uS w/ clk/2
#define SEGMENT_MS               4
#define SEGMENT_TIME             (SEGMENT_MS / 60000.0) // mins


// DRV8711 settings
// NOTE, PWM frequency = 1 / (2 * DTIME + TBLANK + TOFF)
// We have PWM frequency = 1 / (2 * 850nS + 1uS + 6.5uS) ~= 110kHz
#define DRV8711_OFF              12
#define DRV8711_BLANK            (0x32 | DRV8711_BLANK_ABT_bm)
#define DRV8711_DECAY            (DRV8711_DECAY_DECMOD_MIXED | 16)

#define DRV8711_STALL            (DRV8711_STALL_SDCNT_2 | \
                                  DRV8711_STALL_VDIV_4 | 200)
#define DRV8711_DRIVE            (DRV8711_DRIVE_IDRIVEP_50  | \
                                  DRV8711_DRIVE_IDRIVEN_100 | \
                                  DRV8711_DRIVE_TDRIVEP_500 | \
                                  DRV8711_DRIVE_TDRIVEN_500 | \
                                  DRV8711_DRIVE_OCPDEG_1    | \
                                  DRV8711_DRIVE_OCPTH_500)
#define DRV8711_TORQUE            DRV8711_TORQUE_SMPLTH_50
// NOTE, Datasheet suggests 850ns DTIME with the optional gate resistor
// installed.  See page 30 section 8.1.2 of DRV8711 datasheet.
#define DRV8711_CTRL             (DRV8711_CTRL_ISGAIN_10 | \
                                  DRV8711_CTRL_DTIME_850 | \
                                  DRV8711_CTRL_EXSTALL_bm)


// RS485 settings
#define RS485_PORT               USARTD1
#define RS485_DRE_vect           USARTD1_DRE_vect
#define RS485_TXC_vect           USARTD1_TXC_vect
#define RS485_RXC_vect           USARTD1_RXC_vect


// Modbus settings
#define MODBUS_TIMEOUT           100 // ms. response timeout
#define MODBUS_RETRIES           4   // Number of retries before failure
#define MODBUS_BUF_SIZE          18  // Max bytes in rx/tx buffers
#define VFD_QUERY_DELAY          100 // ms


// Serial settings
#define SERIAL_BAUD              USART_BAUD_230400 // 115200
#define SERIAL_PORT              USARTC0
#define SERIAL_DRE_vect          USARTC0_DRE_vect
#define SERIAL_RXC_vect          USARTC0_RXC_vect
#define SERIAL_CTS_THRESH        4


// PWM settings
#define POWER_MAX_UPDATES        SEGMENT_MS

// Input
#define INPUT_BUFFER_LEN         128 // text buffer size (255 max)


// Report
#define REPORT_RATE              250 // ms


// I2C
#define I2C_DEV                  TWIC
#define I2C_ISR                  TWIC_TWIS_vect
#define I2C_ADDR                 0x2b
#define I2C_MAX_DATA             8


// Motor
#define MOTOR_IDLE_TIMEOUT       0.25  // secs, motor off after this time
#define MIN_STEP_CORRECTION      2

#define MIN_VELOCITY             10            // mm/min
#define CURRENT_SENSE_RESISTOR   0.05          // ohms
#define CURRENT_SENSE_REF        2.75          // volts
#define MAX_CURRENT              10            // amps
#define VELOCITY_MULTIPLIER      1000.0
#define ACCEL_MULTIPLIER         1000000.0
#define JERK_MULTIPLIER          1000000.0
#define SYNC_QUEUE_SIZE          2048
#define EXEC_FILL_TARGET         8
#define EXEC_DELAY               250 // ms
#define JOG_STOPPING_UNDERSHOOT  1   // % of stopping distance
