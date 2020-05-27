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
#include "hardware.h"
#include "usart.h"
#include "lcd.h"

#include <avr/interrupt.h>

#include <stdint.h>
#include <stdio.h>


#define RESET_PIN SPI_MOSI_PIN


void rtc_init() {}


static struct {
  uint8_t step_pin;
  uint8_t dir_pin;
  TC0_t *timer;
  volatile int16_t high;
  volatile bool reading;

} channel[4] = {
  {STEP_X_PIN, DIR_X_PIN, &TCC0, 0},
  {STEP_Y_PIN, DIR_Y_PIN, &TCD0, 0},
  {STEP_Z_PIN, DIR_Z_PIN, &TCE0, 0},
  {STEP_A_PIN, DIR_A_PIN, &TCF0, 0},
};


static int reset = 0;


void channel_reset(int i) {channel[i].timer->CNT = channel[i].high = 0;}


#define EVSYS_CHMUX(CH) (&EVSYS_CH0MUX)[CH]
#define EVSYS_CHCTRL(CH) (&EVSYS_CH0CTRL)[CH]


void channel_overflow(int i) {
  if (IN_PIN(channel[i].dir_pin)) channel[i].high--;
  else channel[i].high++;
  channel[i].reading = false;
}


ISR(TCC0_OVF_vect) {channel_overflow(0);}
ISR(TCD0_OVF_vect) {channel_overflow(1);}
ISR(TCE0_OVF_vect) {channel_overflow(2);}
ISR(TCF0_OVF_vect) {channel_overflow(3);}


void channel_update_dir(int i) {
  if (IN_PIN(channel[i].dir_pin)) channel[i].timer->CTRLFSET = TC0_DIR_bm;
  else channel[i].timer->CTRLFCLR = TC0_DIR_bm;
}


ISR(PORTE_INT0_vect) {for (int i = 0; i < 4; i++) channel_update_dir(i);}


int32_t __attribute__ ((noinline)) _channel_read(int i) {
  return (int32_t)channel[i].high << 16 | channel[i].timer->CNT;
}


int32_t channel_read(int i) {
  while (true) {
    channel[i].reading = true;

    int32_t x = _channel_read(i);
    int32_t y = _channel_read(i);

    if (x != y || !channel[i].reading) continue;

    channel[i].reading = false;
    return x;
  }
}


void channel_update_enable(int i) {
  if (IN_PIN(MOTOR_ENABLE_PIN))
    channel[i].timer->CTRLA = TC_CLKSEL_EVCH0_gc + i;
  else channel[i].timer->CTRLA = 0;
}


ISR(PORTF_INT0_vect) {
  for (int i = 0; i < 4; i++) channel_update_enable(i);
  if (!IN_PIN(MOTOR_ENABLE_PIN)) reset = 2;
}


ISR(PORTC_INT0_vect) {reset = 32;}


ISR(TCC1_OVF_vect) {
  if (reset) reset--;

  // Report measured steps
  static int32_t counts[4] = {0, 0, 0, 0};
  bool moving = false;
  bool zero = true;
  for (int i = 0; i < 4; i++) {
    int32_t count = channel_read(i);
    if (count != counts[i]) moving = true;
    if (count) zero = false;
    counts[i] = count;
  }

  if (reset && !zero) {
    for (int i = 0; i < 4; i++) channel_reset(i);
    printf("RESET\n");
    return;
  }

  if (moving)
    printf("%ld,%ld,%ld,%ld\n", counts[0], counts[1], counts[2], counts[3]);
}


static void _splash(uint8_t addr) {
  lcd_init(addr);
  lcd_goto(addr, 5, 1);
  lcd_pgmstr(addr, PSTR("Step Test"));
}


void channel_init(int i) {
  uint8_t step_pin = channel[i].step_pin;
  uint8_t dir_pin = channel[i].dir_pin;

  // Configure I/O
  DIRCLR_PIN(step_pin);
  DIRCLR_PIN(dir_pin);
  PINCTRL_PIN(step_pin) = PORT_SRLEN_bm | PORT_ISC_RISING_gc;
  PINCTRL_PIN(dir_pin)  = PORT_SRLEN_bm | PORT_ISC_BOTHEDGES_gc;

  // Dir change interrupt
  PIN_PORT(dir_pin)->INTCTRL  |= PORT_INT0LVL_HI_gc;
  PIN_PORT(dir_pin)->INT0MASK |= PIN_BM(dir_pin);

  // Events
  EVSYS_CHMUX(i) = PIN_EVSYS_CHMUX(step_pin);
  EVSYS_CHCTRL(i) = EVSYS_DIGFILT_8SAMPLES_gc;

  // Clock
  channel_update_enable(i);
  channel[i].timer->INTCTRLA = TC_OVFINTLVL_HI_gc;

  // Set initial clock direction
  channel_update_dir(i);
}


static void init() {
  cli();

  hw_init();
  usart_init();

  // Motor channels
  for (int i = 0; i < 4; i++) channel_init(i);

  // Motor enable
  DIRCLR_PIN(MOTOR_ENABLE_PIN);
  PINCTRL_PIN(MOTOR_ENABLE_PIN) =
    PORT_SRLEN_bm | PORT_ISC_BOTHEDGES_gc | PORT_OPC_PULLUP_gc;
  PIN_PORT(MOTOR_ENABLE_PIN)->INTCTRL  |= PORT_INT0LVL_HI_gc;
  PIN_PORT(MOTOR_ENABLE_PIN)->INT0MASK |= PIN_BM(MOTOR_ENABLE_PIN);

  // Configure report clock
  TCC1.INTCTRLA = TC_OVFINTLVL_LO_gc;
  TCC1.PER = F_CPU / 256 * 0.01; // 10ms
  TCC1.CTRLA = TC_CLKSEL_DIV256_gc;

  // Reset switch
  DIRCLR_PIN(RESET_PIN);
  PINCTRL_PIN(RESET_PIN) =
    PORT_SRLEN_bm | PORT_ISC_RISING_gc | PORT_OPC_PULLUP_gc;
  PIN_PORT(RESET_PIN)->INTCTRL  |= PORT_INT0LVL_LO_gc;
  PIN_PORT(RESET_PIN)->INT0MASK |= PIN_BM(RESET_PIN);

  printf("RESET\n");

  sei();
}


int main() {
  init();

  _splash(0x27);
  _splash(0x3f);

  while (true) continue;

  return 0;
}
