/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2020, Buildbotics LLC, All rights reserved.

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
#include <stdbool.h>


typedef enum {
  OUT_LO,
  OUT_HI,
  OUT_TRI,
} output_state_t;


/// OUT_<inactive>_<active>
typedef enum {
  OUT_DISABLED,
  OUT_LO_HI,
  OUT_HI_LO,
  OUT_TRI_LO,
  OUT_TRI_HI,
  OUT_LO_TRI,
  OUT_HI_TRI,
} output_mode_t;


void outputs_init();
bool outputs_is_active(uint8_t pin);
void outputs_set_active(uint8_t pin, bool active);
bool outputs_toggle(uint8_t pin);
void outputs_set_mode(uint8_t pin, output_mode_t mode);
output_state_t outputs_get_state(uint8_t pin);
void outputs_stop();
