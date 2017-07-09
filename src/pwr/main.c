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

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>

#include <util/delay.h>

#include <stdbool.h>


#define TEMP_ADC 14

// Port A
#define MOTOR_PIN 3
#define LOAD1_PIN 5
#define LOAD2_PIN 6
#define MOTOR_ADC 0
#define LOAD1_ADC 2
#define LOAD2_ADC 3

// Port B
#define VIN_PIN 0
#define VIN_ADC 5

// Port C
#define VOUT_PIN 2
#define VOUT_ADC 11

#define GATE_PORT PORTC
#define GATE_DDR DDRC
#define GATE1_PIN 0
#define GATE2_PIN 4
#define GATE3_PIN 5

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
  NUM_REGS
} regs_t;


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
#define VR1 34800
#define VR2 1000

  return sample * (VREF / 1024.0 * (VR1 + VR2) / VR2 * 100);
}


inline static uint16_t convert_current(uint16_t sample) {
#define CR1 1000
#define CR2 137

  // TODO This isn't correct
  return sample * (VREF / 1024.0 * (CR1 + CR2) / CR2 * 100);
}


void adc_conversion() {
  uint16_t data = ADC;
  uint8_t ch = ADMUX & 0xf;

  switch (ch) {
  case TEMP_ADC:
    regs[TEMP_REG] = data; // Temp in Kelvin
    ch = VIN_ADC;
    break;

  case VIN_ADC:
    regs[VIN_REG] = convert_voltage(data);
    ch = VOUT_ADC;
    break;

  case VOUT_ADC:
    regs[VOUT_REG] = convert_voltage(data);
    ch = MOTOR_ADC;
    break;

  case MOTOR_ADC:
    regs[MOTOR_REG] = convert_current(data);
    ch = LOAD1_ADC;
    break;

  case LOAD1_ADC:
    regs[LOAD1_REG] = convert_current(data);
    ch = LOAD2_ADC;
    break;

  case LOAD2_ADC:
    regs[LOAD2_REG] = convert_current(data);
    ch = TEMP_ADC;
    break;

  default:
    ch = TEMP_ADC;
    break;
  }

  // Start next conversion
  ADMUX = (ADMUX & 0xf0) | ch;
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
    (0 << PRTIM0) | (1 << PRTIM1) | (0 << PRTWI);

  // IO
  GATE_DDR = (1 << GATE1_PIN) | (1 << GATE2_PIN) | (1 << GATE3_PIN); // Out
  PUEC = 1 << 3; // Pull up reset line

  // Disable digital IO on ADC lines
  DIDR0 = (1 << ADC3D) | (1 << ADC2D) | (1 << ADC0D);
  DIDR1 = (1 << ADC5D);
  DIDR2 = (1 << ADC11D);

  // ADC internal 1.1v, enable, with interrupt, prescale 128
  ADMUX = (1 << REFS1) | (0 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADIE) |
    (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRB = 0;

  // Timer (Clear output A on compare match, Fast PWM, disabled)
  TCCR0A = (1 << COM0A1) | (0 << COM0A0) | (1 << WGM01) | (1 << WGM00);
  TCCR0B = 0 << WGM02;
  OCR0A = 255 * 0.2; // Initial duty cycle

  // SPI, enable, enable address/stop interrupt
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
  init();

  TCCR0A = TCCR0B = 0;
  GATE_DDR = (1 << GATE1_PIN) | (1 << 2);
  GATE_PORT = (1 << GATE1_PIN) | (1 << 2);

  while (true) continue;

  // Start ADC
  adc_conversion();

  // Delayed start
  _delay_ms(100);

  // Enable timer with clk/64
  TCCR0B = (CS02 << 0) | (CS01 << 1) | (CS00 << 1);

  _delay_ms(200);

  while (true) {
    OCR0A = 0xff; // 100% duty cycle

    if (get_reg(VIN_REG) < 11) {
      OCR0A = 0; // 0% duty cycle
      _delay_ms(3000);
    }
  }

  return 0;
}
