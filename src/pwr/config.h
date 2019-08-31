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


#define VERSION 4


// Pins
enum {
  AREF_PIN = PORT_A << 3,
  PA1_PIN, // NC
  PA2_PIN, // NC
  CS1_PIN,
  CS2_PIN,
  CS3_PIN,
  CS4_PIN,
  VOUT_REF_PIN,

  VIN_REF_PIN = PORT_B << 3,
  PWR_MOSI_PIN,
  PWR_MISO_PIN,
  SHUNT_PIN,

  MOTOR_PIN = PORT_C << 3, // IN1
  PWR_SCK_PIN,
  PC2_PIN,                 // NC
  PWR_RESET,
  LOAD2_PIN,               // IN3
  LOAD1_PIN,               // IN4
};


// ADC channels
enum {
  CS1_ADC,  // Motor current
  CS2_ADC,  // Vdd current
  CS3_ADC,  // Load 2 current
  CS4_ADC,  // Load 1 current
  VOUT_ADC, // Motor voltage
  VIN_ADC,  // Input voltage
  NC6_ADC,
  NC7_ADC,
  NC8_ADC,
  NC9_ADC,
  NC10_ADC,
  NC11_ADC,
  NC12_ADC,
  NC13_ADC,
  TEMP_ADC, // Temperature
};


#define CAP_CHARGE_TIME 50 // ms
#define VOLTAGE_MIN 11
#define VOLTAGE_MAX 39
#define CURRENT_MAX 25
#define CURRENT_OVERTEMP 19 // Should read as ~21A but over 11.86A in an error
#define LOAD_OVERTEMP_MAX 10
#define MOTOR_SHUTDOWN_THRESH 15
#define VOLTAGE_SETTLE_COUNT 5
#define VOLTAGE_SETTLE_PERIOD 20 // ms
#define VOLTAGE_SETTLE_TOLERANCE 0.01
#define VOLTAGE_EXP 0.01

#define SHUNT_WATTS 5
#define SHUNT_OHMS  5.1
#define SHUNT_PERIOD 65000 // ms
#define SHUNT_JOULES 25    // Power per shunt period
#define SHUNT_JOULES_PER_MS ((float)SHUNT_JOULES / SHUNT_PERIOD)
#define SHUNT_MIN_V 2

#define VOLTAGE_REF 1.1
#define VOLTAGE_REF_R1 37400
#define VOLTAGE_REF_R2 1000
#define CURRENT_REF_R2 137
#define CURRENT_REF_MUL (100.0 * 2700 / CURRENT_REF_R2) // 2700 from datasheet

#define AVG_SCALE 3
#define BUCKETS (1 << AVG_SCALE)

// Addresses 0x60 to 0x67
#define I2C_ADDR 0x60
#define I2C_MASK 0b00001111

#define I2C_ERROR_BM (1 << TWBE)
#define I2C_DATA_INT_BM (1 << TWDIF)
#define I2C_READ_BM (1 << TWDIR)
#define I2C_ADDRESS_STOP_INT_BM (1 << TWASIF)
#define I2C_ADDRESS_MATCH_BM (1 << TWAS)


typedef enum {
  TEMP_REG,
  VIN_REG,
  VOUT_REG,
  MOTOR_REG,
  LOAD1_REG,
  LOAD2_REG,
  VDD_REG,
  FLAGS_REG,
  VERSION_REG,
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
  LOAD1_SHUTDOWN_FLAG            = 1 << 6,
  LOAD2_SHUTDOWN_FLAG            = 1 << 7,
  MOTOR_UNDER_VOLTAGE_FLAG       = 1 << 8,

  // Sense errors
  MOTOR_VOLTAGE_SENSE_ERROR_FLAG = 1 << 9,
  MOTOR_CURRENT_SENSE_ERROR_FLAG = 1 << 10,
  LOAD1_SENSE_ERROR_FLAG         = 1 << 11,
  LOAD2_SENSE_ERROR_FLAG         = 1 << 12,
  VDD_CURRENT_SENSE_ERROR_FLAG   = 1 << 13,

  // State flags
  POWER_SHUTDOWN_FLAG            = 1 << 14,
};


#define FATAL_FLAGS       0x003f
#define SENSE_ERROR_FLAGS 0x3e00
