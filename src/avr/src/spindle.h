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

#include <stdbool.h>
#include <stdint.h>


typedef enum {
  POWER_IGNORE,
  POWER_FORWARD,
  POWER_REVERSE
} power_state_t;


typedef struct {
  power_state_t state;
  float power;
  uint16_t period; // Used by PWM
} power_update_t;


typedef enum {
  SPINDLE_TYPE_DISABLED,
  SPINDLE_TYPE_PWM,
  SPINDLE_TYPE_HUANYANG,
  SPINDLE_TYPE_CUSTOM,
  SPINDLE_TYPE_AC_TECH,
  SPINDLE_TYPE_NOWFOREVER,
  SPINDLE_TYPE_DELTA_VFD015M21A,
  SPINDLE_TYPE_YL600,
  SPINDLE_TYPE_FR_D700,
  SPINDLE_TYPE_SUNFAR_E300,
  SPINDLE_TYPE_OMRON_MX2,
  SPINDLE_TYPE_V70,
  SPINDLE_TYPE_H100,
  SPINDLE_TYPE_WJ200,
  SPINDLE_TYPE_DMM_DYN4,
  SPINDLE_TYPE_GALT_G200,
  SPINDLE_TYPE_TECO_E510,
  SPINDLE_TYPE_EM60,
  SPINDLE_TYPE_FULING_DZB200,
} spindle_type_t;


typedef void (*deinit_cb_t)();


spindle_type_t spindle_get_type();
void spindle_stop();
void spindle_estop();
void spindle_load_power_updates(power_update_t updates[], float minD,
                                float maxD);
void spindle_update(const power_update_t &update);
void spindle_update_speed();
void spindle_idle();
