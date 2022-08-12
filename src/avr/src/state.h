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

#include "pgmspace.h"

#include <stdbool.h>


typedef enum {
  STATE_READY,
  STATE_ESTOPPED,
  STATE_RUNNING,
  STATE_JOGGING,
  STATE_STOPPING,
  STATE_HOLDING,
} state_t;


typedef enum {
  HOLD_REASON_USER_PAUSE,
  HOLD_REASON_USER_STOP,
  HOLD_REASON_PROGRAM_PAUSE,
  HOLD_REASON_OPTIONAL_PAUSE,
  HOLD_REASON_SWITCH_FOUND,
  HOLD_REASON_SWITCH_NOT_FOUND,
} hold_reason_t;


typedef enum {
  PAUSE_USER,
  PAUSE_PROGRAM,
  PAUSE_PROGRAM_OPTIONAL,
} pause_t;


PGM_P state_get_pgmstr(state_t state);
PGM_P state_get_hold_reason_pgmstr(hold_reason_t reason);

state_t state_get();

bool state_is_flushing();
bool state_is_resuming();

void state_seek_hold(bool found);
void state_holding();
void state_running();
void state_jogging();
void state_idle();
void state_estop();

void state_callback();
