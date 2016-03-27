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

#include "gpio.h"

#include "config.h"

#include <avr/io.h>


static PORT_t *_ports[MOTORS] = {
  &PORT_OUT_X, &PORT_OUT_Y, &PORT_OUT_Z, &PORT_OUT_A
};

uint8_t gpio_read_bit(int port) {return _ports[port]->IN & GPIO1_OUT_BIT_bm;}
void gpio_set_bit_on(int port) {_ports[port]->OUTSET = GPIO1_OUT_BIT_bm;}
void gpio_set_bit_off(int port) {_ports[port]->OUTCLR = GPIO1_OUT_BIT_bm;}
void gpio_set_bit_toggle(int port) {_ports[port]->OUTTGL = GPIO1_OUT_BIT_bm;}
