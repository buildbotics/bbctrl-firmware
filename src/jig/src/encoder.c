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

#include "encoder.h"

#include "pins.h"
#include "config.h"

#include <avr/io.h>


typedef struct {
  int8_t pin_a;
  int8_t pin_b;

  uint16_t position;
  float velocity;
} encoder_t;


encoder_t encoders[] = {
  {ENCODER_X_A_PIN, ENCODER_X_B_PIN},
  {0, 0}
};


void encoder_init() {
  for (int i = 0; encoders[i].pin_a; i++) {
    // Inputs
    DIRCLR_PIN(encoders[i].pin_a);
    DIRCLR_PIN(encoders[i].pin_b);

    // Pullup and level sensing
    PINCTRL_PIN(encoders[i].pin_a) = PORT_OPC_PULLUP_gc | PORT_ISC_LEVEL_gc;
    PINCTRL_PIN(encoders[i].pin_b) = PORT_OPC_PULLUP_gc | PORT_ISC_LEVEL_gc;
  }

  // Event channel
  EVSYS_CH0MUX = EVSYS_CHMUX_PORTD_PIN0_gc;
  EVSYS_CH0CTRL = EVSYS_QDEN_bm | EVSYS_DIGFILT_2SAMPLES_gc;

  // Timer config
  TCC0.CTRLD = TC_EVACT_QDEC_gc | TC_EVSEL_CH0_gc;
  TCC0.PER = ENCODER_LINES * 4 - 1;
  TCC0.CTRLA = TC_CLKSEL_DIV1_gc;  // Enable
}


void encoder_rtc_callback() {
  uint16_t position = TCC0.CNT;

  int32_t delta =
    ((int32_t)position - encoders[0].position) % (ENCODER_LINES * 4);

  encoders[0].position = position;
  encoders[0].velocity = encoders[0].velocity * 0.95 + delta * 5.0; // steps/sec
}


uint16_t encoder_get_position(int i) {return encoders[i].position;}
float encoder_get_velocity(int i) {return encoders[i].velocity;}
