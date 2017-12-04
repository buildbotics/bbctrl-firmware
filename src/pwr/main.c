/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2017 Buildbotics LLC
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

#include "pins.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include <util/delay.h>

#include <stdbool.h>


// Pins
enum {
  AREF_PIN = PORT_A << 3,
  PA1_PIN, // NC
  PA2_PIN, // NC
  CS1_PIN,
  CS2_PIN,
  CS3_PIN,
  CS4_PIN,
  VOUT_REF_PIN,

  VIN_REF_PIN = PORT_B << 3,
  PWR_MOSI_PIN,
  PWR_MISO_PIN,
  SHUNT_PIN,

  MOTOR_PIN = PORT_C << 3, // IN1
  PWR_SCK_PIN,
  PC2_PIN,                 // NC
  PWR_RESET,
  LOAD1_PIN,               // IN3
  LOAD2_PIN,               // IN4
};


// ADC
enum {
  CS1_ADC,  // Motor current
  CS2_ADC,  // Vdd current
  CS3_ADC,  // Load 1 current
  CS4_ADC,  // Load 2 current
  VOUT_ADC, // Motor voltage
  VIN_ADC,  // Input voltage
  NC6_ADC,
  NC7_ADC,
  NC8_ADC,
  NC9_ADC,
  NC10_ADC,
  NC11_ADC,
  NC12_ADC,
  NC13_ADC,
  TEMP_ADC, // Temperature
};

#define I2C_ADDR 0x60
#define I2C_MASK 0b00000111

#define I2C_ERROR_BM (1 << TWBE)
#define I2C_DATA_INT_BM (1 << TWDIF)
#define I2C_READ_BM (1 << TWDIR)
#define I2C_ADDRESS_STOP_INT_BM (1 << TWASIF)
#define I2C_ADDRESS_MATCH_BM (1 << TWAS)


typedef enum {
  TEMP_REG,
  VIN_REG,
  VOUT_REG,
  MOTOR_REG,
  LOAD1_REG,
  LOAD2_REG,
  VDD_REG,
  NUM_REGS
} regs_t;


static const uint8_t ch_schedule[] = {
  TEMP_ADC, VOUT_ADC,
  VIN_ADC,  VOUT_ADC,
  CS1_ADC,  VOUT_ADC,
  CS2_ADC,  VOUT_ADC,
  CS3_ADC,  VOUT_ADC,
  CS4_ADC,  VOUT_ADC,
  0
};

volatile uint16_t regs[NUM_REGS] = {0};


void i2c_ack() {TWSCRB = (1 << TWCMD1) | (1 << TWCMD0);}
void i2c_nack() {TWSCRB = (1 << TWAA) | (1 << TWCMD1) | (1 << TWCMD0);}


ISR(TWI_SLAVE_vect) {
  static uint8_t byte = 0;
  static uint16_t reg;

  // Stretch clock longer to work around RPi bug
  // See https://github.com/raspberrypi/linux/issues/254
  _delay_us(10);

  uint8_t status = TWSSRA;

  if (status & I2C_DATA_INT_BM) {
    if (status & I2C_READ_BM) {
      // send response
      if (byte < 2) {
        switch (byte++) {
        case 0: TWSD = reg; break;
        case 1: TWSD = reg >> 8; break;
        }

        i2c_ack();

      } else i2c_nack();

    } else i2c_ack(); // Write ignore

  } else if (status & I2C_ADDRESS_STOP_INT_BM) {
    if (status & I2C_ADDRESS_MATCH_BM) {
      // read address
      uint8_t addr = (TWSD >> 1) & I2C_MASK;

      if (addr < NUM_REGS) {
        i2c_ack();
        reg = regs[addr];
        byte = 0;

      } else i2c_nack();

    } else TWSCRB = (1 << TWCMD1) | (0 << TWCMD0);  // Stop
  }
}


inline static uint16_t convert_voltage(uint16_t sample) {
#define VREF 1.1
#define VR1 34800 // TODO v10 will have 37.4k
#define VR2 1000

  return sample * (VREF / 1024.0 * (VR1 + VR2) / VR2 * 100);
}


inline static uint16_t convert_current(uint16_t sample) {
  return sample * (VREF / 1024.0 * 1970);
}


static void read_conversion(uint8_t ch) {
  uint16_t data = ADC;

  switch (ch) {
  case TEMP_ADC: regs[TEMP_REG]  = data; break; // in Kelvin
  case VIN_ADC:  regs[VIN_REG]   = convert_voltage(data); break;
  case VOUT_ADC: regs[VOUT_REG]  = convert_voltage(data); break;
  case CS1_ADC:  regs[MOTOR_REG] = convert_current(data); break;
  case CS2_ADC:  regs[VDD_REG]   = convert_current(data); break;
  case CS3_ADC:  regs[LOAD1_REG] = convert_current(data); break;
  case CS4_ADC:  regs[LOAD2_REG] = convert_current(data); break;
  }
}


void adc_conversion() {
  static int8_t i = 0;

  read_conversion(ch_schedule[i]);
  if (!ch_schedule[++i]) i = 0;

  // Start next conversion
  ADMUX = (ADMUX & 0xf0) | ch_schedule[i];
  ADCSRA |= 1 << ADSC;
}


ISR(ADC_vect) {adc_conversion();}


void init() {
  cli();

  // CPU Clock, disable CKOUT
  CCP = 0xd8;
  CLKSR = (1 << CSTR) | (1 << CKOUT_IO) | 0b0010; // 8Mhz internal clock
  CCP = 0xd8;
  CLKPR = 0; // div 1
  while (!((1 << 7) & CLKSR)) continue; // Wait for clock to stabilize

  // Power reduction
  PRR = (0 << PRADC) | (1 << PRUSART0) | (1 << PRUSART1) | (1 << PRUSI) |
    (0 << PRTIM0) | (0 << PRTIM1) | (0 << PRTWI);

  // IO
  IO_DDR_SET(MOTOR_PIN);  // Output
  IO_DDR_SET(LOAD1_PIN);  // Output
  IO_DDR_SET(LOAD2_PIN);  // Output
  IO_PUE_SET(PWR_RESET);  // Pull up reset line

  // Disable digital IO on ADC lines
  DIDR0 = (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D)| (1 << ADC1D) |
    (1 << ADC0D) | (1 << AREFD);
  DIDR1 = (1 << ADC5D);

  // ADC internal 1.1v, enable, with interrupt, prescale 128
  ADMUX = (1 << REFS1) | (0 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADIE) |
    (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRB = 0;

  // Timer 0 (Clear output A on compare match, Fast PWM, disabled)
  TCCR0A = (1 << COM0A1) | (0 << COM0A0) | (1 << WGM01) | (1 << WGM00);
  TCCR0B = 0 << WGM02;

  // Timer 1 (Set output A on compare match, Fast PWM, 8-bit, no prescale)
  OCR1A = 0; // Off
  TCCR1A = (1 << COM1A1) | (1 << COM1A0) | (0 << WGM11) | (1 << WGM10);
  TCCR1B =
    (0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11) | (1 << CS10);

  // I2C, enable, enable address/stop interrupt
  TWSCRA = (1 << TWEN) | (1 << TWASIE) | (1 << TWDIE);
  TWSA = I2C_ADDR << 1;
  TWSAM = I2C_MASK << 1;

  sei();
}


float get_reg(int reg) {
  cli();
  float value = regs[reg];
  sei();

  return value / 100;
}


int main() {
  wdt_enable(WDTO_8S);

  init();

  // Start ADC
  adc_conversion();

  // Validate input voltage
  int settle = 0;
  float vlast = 0;

  while (settle < 5) {
    wdt_reset();
    _delay_ms(20);

    float vin = get_reg(VIN_REG);

    // Check that voltage is with in range and settled
    if (11 < vin && vin < 39 && vlast * 0.98 < vin && vin < vlast * 1.02)
      settle++;
    else settle = 0;

    vlast = vin;
  }

  // Charge caps
  OCR0A = 255 * 0.2; // Cap charging duty cycle
  TCCR0B |= (0 << CS02) | (1 << CS01) | (0 << CS00); // Enable timer with clk/8
  _delay_ms(200);
  TCCR0A = 0; // Clock off
  IO_PORT_SET(MOTOR_PIN); // Motor voltage on

  _delay_ms(50); // Wait for final charge

  // Measure nominal voltage
  float vnom = 0;
  settle = 0;
  while (settle < 5) {
    wdt_reset();
    _delay_ms(20);

    float vout = get_reg(VOUT_REG);

    // Check that voltages are with in range and vout has settled
    if (11 < vout && vout < 39 && vout * 0.98 < vnom && vnom < vout * 1.02)
      settle++;
    else settle = 0;

    vnom = vout;
  }

  if (36 < vnom) vnom = 36; // TODO remove this when R27 is updated

  bool shunt = false;

  while (true) {
    wdt_reset();
    float vout = get_reg(VOUT_REG);

    if (!shunt && vnom + 2 < vout) {
      shunt = true;
      IO_DDR_SET(SHUNT_PIN); // Enable output

    } else if (shunt && vout < vnom + 1) {
      IO_DDR_CLR(SHUNT_PIN); // Disable output
      shunt = false;
    }

    if (shunt) {
      float duty = (vout - vnom - 1) / 4;
      if (1 < duty) OCR1A = 0xff;
      else OCR1A = 0xff * duty;
    }

    continue;

    if (39 < get_reg(VIN_REG) || get_reg(VIN_REG) < 11) {
      IO_PORT_CLR(MOTOR_PIN);
      _delay_ms(3000);
      IO_PORT_SET(MOTOR_PIN); // Motor voltage on
    }

    if (10 < get_reg(MOTOR_REG)) {
      IO_PORT_CLR(MOTOR_PIN);
      IO_PORT_CLR(LOAD1_PIN);
      IO_PORT_CLR(LOAD2_PIN);
      _delay_ms(1000);
      IO_PORT_SET(MOTOR_PIN); // Motor voltage on
    }
  }

  return 0;
}
