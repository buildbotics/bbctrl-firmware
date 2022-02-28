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

#include <stdint.h>


// Must be kept in synch with resources/config-template.json
typedef enum {
  IO_DISABLED,

  // Mappable functions
  INPUT_MOTOR_0_MAX, INPUT_MOTOR_1_MAX, INPUT_MOTOR_2_MAX, INPUT_MOTOR_3_MAX,
  INPUT_MOTOR_0_MIN, INPUT_MOTOR_1_MIN, INPUT_MOTOR_2_MIN, INPUT_MOTOR_3_MIN,
  INPUT_0, INPUT_1, INPUT_2, INPUT_3, INPUT_ESTOP, INPUT_PROBE,
  OUTPUT_0, OUTPUT_1, OUTPUT_2, OUTPUT_3, OUTPUT_MIST, OUTPUT_FLOOD,
  OUTPUT_FAULT, OUTPUT_TOOL_ENABLE, OUTPUT_TOOL_DIRECTION,
  ANALOG_0, ANALOG_1, ANALOG_2, ANALOG_3,

  // Hard wired functions
  INPUT_STALL_0, INPUT_STALL_1, INPUT_STALL_2, INPUT_STALL_3, INPUT_MOTOR_FAULT,
  OUTPUT_TEST,

  IO_FUNCTION_COUNT
} io_function_t;


// Must be kept in synch with resources/config-template.json
// <inactive>_<active>
typedef enum {
  LO_HI, HI_LO, TRI_LO, TRI_HI, LO_TRI, HI_TRI, NORMALLY_CLOSED, NORMALLY_OPEN
} io_mode_t;


typedef enum {
  IO_TYPE_DISABLED = 0,
  IO_TYPE_INPUT    = 1 << 0,
  IO_TYPE_OUTPUT   = 1 << 1,
  IO_TYPE_ANALOG   = 1 << 2,
} io_type_t;


typedef enum {IO_LO, IO_HI, IO_TRI = 0xff} io_level_t;


typedef void (*io_callback_t)(io_function_t function, bool active);


void io_init();
bool io_is_enabled(io_function_t function);
io_type_t io_get_type(io_function_t function);
void io_set_callback(io_function_t function, io_callback_t cb);
io_callback_t io_get_callback(io_function_t function);
void io_set_output(io_function_t function, bool active);
void io_stop_outputs();
io_function_t io_get_port_function(bool digital, uint8_t port);
uint8_t io_get_input(io_function_t function);
float io_get_analog(io_function_t function);
void io_rtc_callback();
