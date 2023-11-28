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

#include "estop.h"
#include "motor.h"
#include "stepper.h"
#include "spindle.h"
#include "io.h"
#include "hardware.h"
#include "config.h"
#include "state.h"
#include "jog.h"
#include "exec.h"


static stat_t estop_reason = STAT_OK;


static void _input_callback(io_function_t function, bool active) {
  if (active) estop_trigger(STAT_ESTOP_SWITCH);
  else estop_clear();
}


void estop_init() {
  io_set_callback(INPUT_ESTOP, _input_callback);
  if (io_get_input(INPUT_ESTOP)) estop_trigger(STAT_ESTOP_SWITCH);
}


bool estop_triggered() {return estop_reason != STAT_OK;}


void estop_trigger(stat_t reason) {
  if (estop_triggered()) return;
  estop_reason = reason;

  // Set fault signal
  io_set_output(OUTPUT_FAULT, true);

  // Shutdown peripherals
  st_shutdown();
  spindle_estop();
  jog_stop();
  io_stop_outputs();

  // Set machine state
  state_estop();
}


void estop_clear() {
  // It is important that we don't clear the estop if it's not set because
  // it can cause a reboot loop.
  if (!estop_triggered()) return;

  // Check if estop is set
  if (io_get_input(INPUT_ESTOP)) {
    estop_reason = STAT_ESTOP_SWITCH;
    return; // Can't clear while estop is still active
  }

  // Clear fault signal
  io_set_output(OUTPUT_FAULT, false);

  estop_reason = STAT_OK;

  // Reboot
  // Note, hardware.c waits until any spindle stop command has been delivered
  hw_request_hard_reset();
}


// Var callbacks
bool get_estop() {return estop_triggered();}


void set_estop(bool value) {
  if (value == estop_triggered()) return;
  if (value) estop_trigger(STAT_ESTOP_USER);
  else estop_clear();
}


PGM_P get_estop_reason() {return status_to_pgmstr(estop_reason);}


// Command callbacks
stat_t command_estop(char *cmd) {
  estop_trigger(STAT_ESTOP_USER);
  return STAT_OK;
}


stat_t command_shutdown(char *cmd) {
  estop_trigger(STAT_POWER_SHUTDOWN);
  return STAT_OK;
}


stat_t command_clear(char *cmd) {estop_clear(); return STAT_OK;}
