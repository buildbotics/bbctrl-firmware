/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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

#include <avr/io.h>


#define VERSION 0x0104

#define VIN_ADC  ADC_MUXPOS_AIN9_gc // ACD0
#define VOUT_ADC ADC_MUXPOS_AIN8_gc // ADC0
#define IMON_ADC ADC_MUXPOS_AIN0_gc // ADC1
#define TEMP_ADC ADC_MUXPOS_TEMPSENSE_gc

#define VIN_OFFSET 0.3 // volts, due to diode drop
#define VSCALE 0.9725  // Linear voltage measurement correction

#define REFV   4.43
#define VR1    100000.0
#define VR2    7020.0
#define RSENSE 0.002
#define ISCALE 100 // Current sense scale

#define MOTOR_PORT PORTC
#define MOTOR_PIN (1 << 1)
#define SHUNT_PORT PORTA
#define SHUNT_PIN (1 << 5)
#define DEBUG_PORT PORTA
#define DEBUG_PIN (1 << 6)
#define DRVEN_PORT PORTC
#define DRVEN_PIN (1 << 3)

#define SHUNT_FAIL_VOLTAGE       5
#define MAX_DISCHARGE_WAIT_TIME  500 // ms/volt
#define MAX_DUMP_WAIT_TIME       1   // ms/volt
#define MAX_CHARGE_WAIT_TIME     30  // ms/volt

#define GATE_TURN_ON_DELAY       40
#define CAP_DISCHARGE_TIME       20  // ms
#define VOLTAGE_MIN              20
#define VOLTAGE_MAX              53
#define CURRENT_MAX              25
#define MOTOR_DRIVER_CURRENT     0.15
#define VOLTAGE_SETTLE_COUNT     5
#define VOLTAGE_SETTLE_PERIOD    20 // ms
#define VOLTAGE_SETTLE_TOLERANCE 0.01
#define VOLTAGE_EXP              0.01

#define SHUNT_WATTS              5
#define SHUNT_OHMS               5.1
#define SHUNT_PERIOD             65000 // ms
#define SHUNT_JOULES             25    // Power per shunt period
#define SHUNT_JOULES_PER_MS      ((float)SHUNT_JOULES / SHUNT_PERIOD)
#define SHUNT_MIN_V              2

#define REG_SCALE                100
#define ADC_SAMPNUM              ADC_SAMPNUM_ACC4_gc
#define ADC_SAMPLES              (1 << ADC_SAMPNUM)

#define ADC_SCALE                5
#define ADC_BUCKETS              (1 << ADC_SCALE)

#define I2C_ADDR                 0x5f


typedef enum {
  TEMP_REG,
  VIN_REG,
  VOUT_REG,
  MOTOR_REG,
  RESERVED1_REG,
  RESERVED2_REG,
  RESERVED3_REG,
  FLAGS_REG,
  VERSION_REG,
  CRC_REG,
  NUM_REGS
} regs_t;


enum {
  // Fatal
  UNDER_VOLTAGE_FLAG             = 1 << 0,
  OVER_VOLTAGE_FLAG              = 1 << 1,
  OVER_CURRENT_FLAG              = 1 << 2,
  SENSE_ERROR_FLAG               = 1 << 3,
  SHUNT_OVERLOAD_FLAG            = 1 << 4,
  MOTOR_OVERLOAD_FLAG            = 1 << 5,

  // Non fatal
  GATE_ERROR_FLAG                = 1 << 6,
  NOT_INITIALIZED_FLAG           = 1 << 7,
  MOTOR_UNDER_VOLTAGE_FLAG       = 1 << 8,
  CHARGE_ERROR_FLAG              = 1 << 11,
  SHUNT_ERROR_FLAG               = 1 << 15,

  // Sense errors
  MOTOR_VOLTAGE_SENSE_ERROR_FLAG = 1 << 9,
  MOTOR_CURRENT_SENSE_ERROR_FLAG = 1 << 10,

  // State flags
  POWER_SHUTDOWN_FLAG            = 1 << 14,
};


#define FATAL_FLAGS       0x003f
#define SENSE_ERROR_FLAGS 0x0600
