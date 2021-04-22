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

#include <config.h>

#include <avr/io.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>


void __SPIC_INT_vect();      // DRV8711 SPI
void __I2C_ISR();            // I2C from RPi
void __ADCA_CH0_vect();      // Analog input
void __ADCA_CH1_vect();      // Analog input
void __RS485_DRE_vect();     // RS848
void __RS485_TXC_vect();     // RS848
void __RS485_RXC_vect();     // RS848
void __SERIAL_DRE_vect();    // Serial to RPi
void __SERIAL_RXC_vect();    // Serial from RPi
void __STEP_LOW_LEVEL_ISR(); // Stepper lo interrupt
void __STEP_TIMER_ISR();     // Stepper hi interrupt
void __RTC_OVF_vect();       // RTC tick

void motor_emulate_steps(int motor);

extern int __argc;
extern char **__argv;


volatile uint8_t io_mem[4096] = {0};


bool fast = false;
int serialByte = -1;
uint8_t i2cData[I2C_MAX_DATA];
int i2cIndex = 0;
bool haveI2C = false;
fd_set readFDs;


void cli() {}
void sei() {}


void emu_init() {
  // Parse command line args
  for (int i = 0; i < __argc; i++)
    if (strcmp(__argv[i], "--fast") == 0) fast = true;

  // Mark clocks ready
  OSC.STATUS = OSC_XOSCRDY_bm | OSC_PLLRDY_bm | OSC_RC32KRDY_bm;

  // So usart_flush() returns
  SERIAL_PORT.STATUS = USART_DREIF_bm | USART_TXCIF_bm;

  // Clear motor fault
  PIN_PORT(MOTOR_FAULT_PIN)->IN |= PIN_BM(MOTOR_FAULT_PIN);

  FD_ZERO(&readFDs);
}


void emu_callback() {
  fflush(stdout);

  if (RST.CTRL == RST_SWRST_bm) exit(0);

  struct timeval t = {0, fast ? 0 : 1000};
  bool readData = true;
  while (readData) {
    readData = false;

    // Try to read
    FD_SET(0, &readFDs);
    if (fcntl(3, F_GETFL) != -1) FD_SET(3, &readFDs);

    if (0 < select(4, &readFDs, 0, 0, &t)) {
      uint8_t data;

      if (serialByte == -1 && FD_ISSET(0, &readFDs) && read(0, &data, 1) == 1)
        serialByte = data;

      if (!haveI2C && FD_ISSET(3, &readFDs) && read(3, &data, 1) == 1) {
        if (data == '\n') haveI2C = true;
        else if (i2cIndex < I2C_MAX_DATA) i2cData[i2cIndex++] = data;
      }
    }

    // Send message to i2c port
    if (haveI2C && (I2C_DEV.SLAVE.CTRLA & TWI_SLAVE_INTLVL_LO_gc)) {
      // START
      I2C_DEV.SLAVE.STATUS = TWI_SLAVE_APIF_bm | TWI_SLAVE_AP_bm;
      __I2C_ISR();

      // DATA
      for (int i = 0; i < i2cIndex; i++) {
        I2C_DEV.SLAVE.STATUS = TWI_SLAVE_DIF_bm;
        I2C_DEV.SLAVE.DATA = i2cData[i];
        __I2C_ISR();
      }

      // STOP
      I2C_DEV.SLAVE.STATUS = TWI_SLAVE_APIF_bm;
      __I2C_ISR();

      i2cIndex = 0;
      haveI2C = false;
      readData = true;
    }

    // Send byte to serial port
    if (serialByte != -1 && SERIAL_PORT.CTRLA & USART_RXCINTLVL_MED_gc) {
      SERIAL_PORT.DATA = (uint8_t)serialByte;
      __SERIAL_RXC_vect();

      if (SERIAL_PORT.CTRLA & USART_RXCINTLVL_MED_gc) {
        serialByte = -1;
        readData = true;
      }
    }
  }

  // Call stepper ISRs
  if (ADCB_CH0_INTCTRL == ADC_CH_INTLVL_LO_gc) __STEP_LOW_LEVEL_ISR();
  for (int motor = 0; motor < 4; motor++) motor_emulate_steps(motor);
  __STEP_TIMER_ISR();

  // Call RTC
  __RTC_OVF_vect();

  // Throttle with remaining time
  if (t.tv_usec) usleep(t.tv_usec);
}
