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

#pragma once

#include <stdint.h>


enum {SPINDLE_LED, SPINDLE_DIR_LED, SPINDLE_PWM_LED, COOLANT_LED};

void indicator_led_set();
void indicator_led_clear();
void indicator_led_toggle();

void gpio_led_on(uint8_t led);
void gpio_led_off(uint8_t led);
void gpio_led_toggle(uint8_t led);

uint8_t gpio_read_bit(uint8_t b);
void gpio_set_bit_on(uint8_t b);
void gpio_set_bit_off(uint8_t b);
void gpio_set_bit_toggle(uint8_t b);
