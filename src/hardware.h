/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
                  Copyright (c) 2012 - 2015 Rob Giseburt
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

#include "status.h"
#include "config.h"

/*
  Device singleton - global structure to allow iteration through similar devices

  Ports are shared between steppers and GPIO so we need a global struct.
  Each xmega port has 3 bindings; motors, switches and the output bit

  Care needs to be taken in routines that use ports not to write to bits that
  are not assigned to the designated function - ur unpredicatable results will
  occur.
*/
typedef struct {
  PORT_t *st_port[MOTORS];  // bindings for stepper motor ports (stepper.c)
  PORT_t *sw_port[MOTORS];  // bindings for switch ports (GPIO2)
  PORT_t *out_port[MOTORS]; // bindings for output ports (GPIO1)
} hwSingleton_t;
extern hwSingleton_t hw;


void hardware_init();
void hw_get_id(char *id);
void hw_request_hard_reset();
void hw_hard_reset();
stat_t hw_hard_reset_handler();

void hw_request_bootloader();
stat_t hw_bootloader_handler();
