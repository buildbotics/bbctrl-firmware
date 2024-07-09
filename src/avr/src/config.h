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

#pragma once

#include "pins.h"

#include <avr/interrupt.h>


// Pins
enum {
  STALL_0_PIN = PIN_ID(PORT_A, 0),
  STALL_1_PIN,
  STALL_2_PIN,
  STALL_3_PIN,
  IO_16_PIN,
  IO_15_PIN,
  IO_24_PIN,
  IO_18_PIN,

  IO_03_PIN = PIN_ID(PORT_B, 0),
  IO_04_PIN,
  IO_11_PIN,
  IO_12_PIN,
  IO_05_PIN,
  IO_08_PIN,
  IO_09_PIN,
  IO_10_PIN,

  SDA_PIN = PIN_ID(PORT_C, 0),
  SCL_PIN,
  SERIAL_RX_PIN,
  SERIAL_TX_PIN,
  SERIAL_CTS_PIN,
  SPI_CLK_PIN,
  SPI_MISO_PIN,
  SPI_MOSI_PIN,

  STEP_0_PIN = PIN_ID(PORT_D, 0),
  SPI_CS_0_PIN,
  SPI_CS_3_PIN,
  SPI_CS_2_PIN,
  PWM_PIN,
  IO_01_PIN,
  RS485_RO_PIN,
  RS485_DI_PIN,

  STEP_1_PIN = PIN_ID(PORT_E, 0),
  SPI_CS_1_PIN,
  DIR_0_PIN,
  DIR_1_PIN,
  STEP_3_PIN,
  IO_02_PIN,
  DIR_2_PIN,
  DIR_3_PIN,

  STEP_2_PIN = PIN_ID(PORT_F, 0),
  RS485_RW_PIN,
  IO_21_PIN,
  IO_23_PIN,
  MOTOR_FAULT_PIN,
  BUFFER_ENABLE_PIN,
  TEST_PIN,
  IO_22_PIN,
};

#define SPI_SS_PIN               SERIAL_CTS_PIN // Needed for SPI configuration


#define AXES                     6 // number of axes
#define MOTORS                   4 // number of motors on the board
#define INS                      6 // number of supported pin outputs
#define OUTS                    10 // number of supported pin outputs
#define ANALOGS                  2 // number of supported analog inputs
#define DIGITALS                 4 // number of supported digital inputs
#define VFDREG                  32 // number of supported VFD modbus registers
#define IO_PINS                 17 // number of supported i/o pins

// Input settings.  See io.c
#define INPUT_DEBOUNCE          5 // ms, default value
#define INPUT_LOCKOUT         250 // ms, default value
#define INPUT_MAX_DEBOUNCE   5000 // ms
#define INPUT_MAX_LOCKOUT   60000 // ms


// Motor ISRs
#define STALL_ISR_vect           PORTA_INT1_vect
#define FAULT_ISR_vect           PORTF_INT1_vect


/* Interrupt usage:
 *
 *    HI    Step timers                          stepper.c
 *    HI    Serial RX                            usart.c
 *   MED    Serial TX                            usart.c (* see note)
 *   MED    Modbus serial interrupts             modbus.c
 *    LO    Segment execution SW interrupt       stepper.c
 *    LO    I2C Slave                            i2c.c
 *    LO    Real-time clock interrupt            rtc.c
 *    LO    DRV8711 SPI                          drv8711.c
 *    LO    A2D interrupts                       analog.c
 *
 *    (*) The TX cannot run at LO level or exception reports and other prints
 *        called from a LO interrupt (as in prep_line()) will kill the system
 *        in a permanent loop call in usart_putc() (usart.c).
 */

// Timer assignments
// NOTE, TCC1 is unused
#define TIMER_STEP               TCC0 // Step timer (see stepper.h)
#define TIMER_PWM                TCD1 // PWM timer  (see pwm.c)


// Timer setup for stepper and dwells
#define STEP_TIMER_DIV           8
#define STEP_TIMER_FREQ          (F_CPU / STEP_TIMER_DIV)
#define STEP_TIMER_POLL          ((uint16_t)(STEP_TIMER_FREQ * 0.001)) // 1ms
#define STEP_TIMER_ISR           TCC0_OVF_vect
#define STEP_LOW_LEVEL_ISR       ADCB_CH0_vect
#define STEP_PULSE_WIDTH         (F_CPU * 0.000002) // 2uS w/ clk/1
#define SEGMENT_MS               4
#define SEGMENT_TIME             (SEGMENT_MS / 60000.0) // mins


// DRV8711 settings
// NOTE,   PWM frequency = 1 / (2 * DTIME + TBLANK +   TOFF)
// We have PWM frequency = 1 / (2 * 850nS +    1uS +  6.5uS) ~= 110kHz
// We have PWM frequency = 1 / (2 * 850nS +    1uS + 24.5uS) ~= 36.8kHz
#define DRV8711_OFF              48
#define DRV8711_BLANK            (0x32 | DRV8711_BLANK_ABT_bm)

// NOTE, slow decay mode can cause FET failure when motor wired incorrectly
#define DRV8711_DECAY            DRV8711_DECAY_DECMOD_FAST

#define DRV8711_DRIVE            (DRV8711_DRIVE_IDRIVEP_200 | \
                                  DRV8711_DRIVE_IDRIVEN_400 | \
                                  DRV8711_DRIVE_TDRIVEP_500 | \
                                  DRV8711_DRIVE_TDRIVEN_500 | \
                                  DRV8711_DRIVE_OCPDEG_1    | \
                                  DRV8711_DRIVE_OCPTH_250)
// NOTE, Datasheet suggests 850ns DTIME with the optional gate resistor
// installed.  See page 30 section 8.1.2 of DRV8711 datasheet.
#define DRV8711_CTRL             (DRV8711_CTRL_ISGAIN_5 | \
                                  DRV8711_CTRL_DTIME_850)


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
#define I2C_MAX_DATA             16


// Motor
#define MOTOR_IDLE_TIMEOUT       0.25  // secs, motor off after this time
#define MIN_STEP_CORRECTION      2

#define MIN_VELOCITY             10            // mm/min
#define CURRENT_SENSE_RESISTOR   0.05          // ohms
#define CURRENT_SENSE_REF        2.75          // volts
#define MAX_IDLE_CURRENT         2             // amps
#define VELOCITY_MULTIPLIER      1000.0
#define ACCEL_MULTIPLIER         1000000.0
#define JERK_MULTIPLIER          1000000.0
#define SYNC_QUEUE_SIZE          4096
#define EXEC_FILL_TARGET         8
#define EXEC_DELAY               250 // ms
#define JOG_STOPPING_UNDERSHOOT  1   // % of stopping distance
