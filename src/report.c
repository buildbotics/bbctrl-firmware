/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include "report.h"
#include "config.h"
#include "canonical_machine.h"
#include "usart.h"
#include "rtc.h"
#include "vars.h"

#include "plan/planner.h"

#include <avr/pgmspace.h>

#include <stdio.h>
#include <stdbool.h>


static bool report_requested = false;
static bool report_full = false;
static uint32_t last_report = 0;


void report_request() {
  report_requested = true;
}


void report_request_full() {
  report_requested = report_full = true;
}


stat_t report_callback() {
  uint32_t now = rtc_get_time();
  if (now - last_report < 100) return STAT_OK;
  last_report = now;

  if (report_requested && usart_tx_empty()) vars_report(report_full);
  report_requested = report_full = false;

  return STAT_OK;
}


float get_position(int index) {
  return cm_get_absolute_position(0, index);
}


float get_velocity() {
  return mp_get_runtime_velocity();
}
