/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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

#include "usart.h"
#include "plan/planner.h"
#include "plan/buffer.h"

#include <avr/pgmspace.h>


float get_position(int index) {return mp_get_runtime_absolute_position(index);}
float get_velocity() {return mp_get_runtime_velocity();}
bool get_echo() {return usart_is_set(USART_ECHO);}
void set_echo(bool value) {return usart_set(USART_ECHO, value);}
uint16_t get_queue() {return mp_get_planner_buffer_room();}
int32_t get_line() {return mr.ms.line;}


PGM_P get_state() {
  switch (mach_get_state()) {
  case STATE_IDLE:     return PSTR("idle");
  case STATE_ESTOP:    return PSTR("estop");
  case STATE_RUNNING:  return PSTR("running");
  case STATE_HOMING:   return PSTR("homing");
  case STATE_PROBING:  return PSTR("probing");
  case STATE_STOPPING: return PSTR("stopping");
  case STATE_HOLDING:  return PSTR("holding");
  }

  return PSTR("invalid");
}
