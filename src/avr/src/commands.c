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

#include "config.h"
#include "stepper.h"
#include "command.h"
#include "vars.h"
#include "base64.h"
#include "hardware.h"
#include "report.h"
#include "exec.h"

#include <stdio.h>


stat_t command_dwell(char *cmd) {
  float seconds;
  if (!b64_decode_float(cmd + 1, &seconds)) return STAT_BAD_FLOAT;
  command_push(*cmd, &seconds);
  return STAT_OK;
}


static stat_t _dwell_exec() {
  exec_set_cb(0); // Immediately clear the callback
  return STAT_OK;
}


unsigned command_dwell_size() {return sizeof(float);}


void command_dwell_exec(void *seconds) {
  st_prep_dwell(*(float *)seconds);
  exec_set_cb(_dwell_exec); // Command must set an exec callback
}


stat_t command_help(char *cmd) {
  printf_P(PSTR("\n{\"commands\":{"));
  command_print_json();
  printf_P(PSTR("},\"variables\":{"));
  vars_print_json();
  printf_P(PSTR("}}\n"));

  return STAT_OK;
}


stat_t command_reboot(char *cmd) {
  hw_request_hard_reset();
  return STAT_OK;
}


stat_t command_dump(char *cmd) {
  report_request_full();
  return STAT_OK;
}
