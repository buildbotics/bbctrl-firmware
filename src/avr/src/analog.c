/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2021, Buildbotics LLC, All rights reserved.

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
