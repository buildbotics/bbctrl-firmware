/******************************************************************************\

                 This file is part of the Buildbotics firmware.

                   Copyright (c) 2015 - 2018, Buildbotics LLC
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

#include "analog.h"

#include "config.h"

#include <avr/interrupt.h>

#include <stdint.h>


typedef struct {
  uint8_t pin;
  uint16_t value;
} analog_port_t;


analog_port_t ports[] = {
  {.pin = ANALOG_1_PIN},
  {.pin = ANALOG_2_PIN},
};


ISR(ADCA_CH0_vect) {ports[0].value = ADCA.CH0.RES;}
ISR(ADCA_CH1_vect) {ports[1].value = ADCA.CH1.RES;}


void analog_init() {
  // Channel 0
  ADCA.CH0.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;
  ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN6_gc;
  ADCA.CH0.INTCTRL = ADC_CH_INTLVL_LO_gc;

  // Channel 1
  ADCA.CH1.CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_SINGLEENDED_gc;
  ADCA.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN7_gc;
  ADCA.CH1.INTCTRL = ADC_CH_INTLVL_LO_gc;

  // ADC
  ADCA.REFCTRL = ADC_REFSEL_INTVCC_gc; // 3.3V / 1.6 = 2.06V
  ADCA.PRESCALER = ADC_PRESCALER_DIV512_gc;
  ADCA.EVCTRL = ADC_SWEEP_01_gc;
  ADCA.CTRLA = ADC_FLUSH_bm | ADC_ENABLE_bm;
}


float analog_get(unsigned port) {
  if (1 < port) return 0;
  return ports[port].value * (1.0 / 0x1000);
}


void analog_rtc_callback() {
  static uint8_t count = 0;

  // Every 1/4 sec
  if (++count == 250) {
    count = 0;
    ADCA.CTRLA |= ADC_CH0START_bm | ADC_CH1START_bm;
  }
}


// Var callbacks
float get_analog_input(int port) {return analog_get(port);}
