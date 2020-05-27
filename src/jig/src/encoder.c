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
