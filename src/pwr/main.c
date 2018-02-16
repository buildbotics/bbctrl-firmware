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

#include "config.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/wdt.h>

#include <util/delay.h>

#include <stdbool.h>


typedef struct {
  const regs_t reg;
  const uint8_t pin;
  volatile uint8_t limit;
  volatile uint8_t count;
  volatile uint8_t lockout;
} load_t;


load_t loads[2] = {
  {LOAD1_REG, LOAD1_PIN, 0, 0, 0},
  {LOAD2_REG, LOAD2_PIN, 0, 0, 0},
};


static const uint8_t ch_schedule[] = {
  TEMP_ADC, VOUT_ADC,
  VIN_ADC,  VOUT_ADC,
  CS1_ADC,  VOUT_ADC,
  CS2_ADC,  VOUT_ADC,
  CS3_ADC,  VOUT_ADC,
  CS4_ADC,  VOUT_ADC,
};


static volatile uint16_t regs[NUM_REGS] = {0};
static volatile uint64_t time = 0; // ms
static volatile uint8_t motor_overload = 0;
static volatile bool shunt_overload = false;
static volatile float shunt_ms_power = 0;
static volatile float vnom = 0;


void i2c_ack() {TWSCRB = (1 << TWCMD1) | (1 << TWCMD0);}
void i2c_nack() {TWSCRB = (1 << TWAA) | (1 << TWCMD1) | (1 << TWCMD0);}


ISR(TWI_SLAVE_vect) {
  static uint8_t byte = 0;
  static uint16_t reg;

  // Stretch clock longer to work around RPi bug
  // See https://github.com/raspberrypi/linux/issues/254
  _delay_us(10); // Must use software delay while in interrupt

  uint8_t status = TWSSRA;

  if (status & I2C_DATA_INT_BM) {
    if (status & I2C_READ_BM) {
      // Send response
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
      // Read address
      uint8_t addr = (TWSD >> 1) & I2C_MASK;

      if (addr < NUM_REGS) {
        i2c_ack();
        reg = regs[addr];
        byte = 0;

      } else i2c_nack();

    } else TWSCRB = (1 << TWCMD1) | (0 << TWCMD0);  // Stop
  }
}


static bool limited_counter(volatile uint8_t *counter, bool up, uint8_t max) {
  if (up) {
    if (*counter < max) (*counter)++;
  } else if (0 < *counter) (*counter)--;

  return *counter == max;
}


static float get_total_current() {
  cli();
  float current =
    regs[MOTOR_REG] + regs[VDD_REG] + regs[LOAD1_REG] + regs[LOAD2_REG];
  sei();

  return current / 100;
}


static float get_reg(int reg) {
  cli();
  float value = regs[reg];
  sei();

  return value / 100;
}


static void update_shunt() {
  static float watts = SHUNT_WATTS;

  // Add power dissipation credit
  watts += SHUNT_WATTS / 1000.0;
  if (SHUNT_WATTS < watts) watts = SHUNT_WATTS;

  // Remove power dissipation credit
  watts -= shunt_ms_power;
  if (watts < 0) shunt_overload = true;

  // Enable shunt when requested
  if (shunt_ms_power) IO_DDR_SET(SHUNT_PIN); // Enable
  else IO_DDR_CLR(SHUNT_PIN);                // Disable
}


static void update_shunt_power(float vout, float vnom) {
  if (vnom + SHUNT_MIN_V < vout) {
    float duty = (vout - vnom - SHUNT_MIN_V) / SHUNT_MAX_V;
    if (duty < 0) duty = 0;
    if (1 < duty) duty = 1;

    // Compute the power credits used per ms
    shunt_ms_power = vout * vout * duty * (1.0 / SHUNT_OHMS / 1000.0);
    OCR1A = 0xff * duty;

  } else shunt_ms_power = 0;
}


static void measure_nominal_voltage() {
  float vin = get_reg(VIN_REG);
  float v;

  if (vnom < VOLTAGE_MIN) v = vin;
  else v = vnom * (1 - VOLTAGE_EXP) + vin * VOLTAGE_EXP;

  vnom = v;
}


static void check_load(load_t *load) {
  // Check overtemp
  bool overtemp = CURRENT_OVERTEMP * 100 < regs[load->reg];
  if (overtemp && !load->lockout) {
    load->lockout = 64;
    if (load->limit < LOAD_LIMIT_TICKS) load->limit++;
  }

  if (load->lockout) load->lockout--;
}


void limit_load(load_t *load) {
  // Limit
  if (load->count < load->limit) {
    IO_PORT_CLR(load->pin); // Lo
    IO_DDR_SET(load->pin);  // Output

  } else IO_DDR_CLR(load->pin); // Float

  if (++load->count == LOAD_LIMIT_TICKS) load->count = 0;
}


ISR(TIMER0_OVF_vect) {
  static uint8_t tick = 0;

  // Calling these too fast disrupts the I2C bus
  if ((tick & 3) == 0) limit_load(&loads[0]);
  if ((tick & 3) == 2) limit_load(&loads[1]);

  if (++tick == 31) {
    time++;
    tick = 0;

    update_shunt(); // Every 1ms
    if (!(time & 7)) measure_nominal_voltage(); // Every 8ms
  }
}


static void delay_ms(uint16_t ms) {
  uint64_t start = time;
  while (time < start + ms) continue;
}


inline static uint16_t convert_voltage(uint16_t sample) {
  return sample * (VOLTAGE_REF / 1024.0 *
                   (VOLTAGE_REF_R1 + VOLTAGE_REF_R2) / VOLTAGE_REF_R2 * 100);
}


inline static uint16_t convert_current(uint16_t sample) {
  return sample * (VOLTAGE_REF / 1024.0 * CURRENT_REF_MUL);
}


static void read_conversion(uint8_t ch) {
  uint16_t data = ADC;

  switch (ch) {
  case TEMP_ADC: regs[TEMP_REG] = data; break; // in Kelvin
  case VIN_ADC:  regs[VIN_REG]  = convert_voltage(data); break;
  case VOUT_ADC: regs[VOUT_REG] = convert_voltage(data); break;

  case CS1_ADC: {
    regs[MOTOR_REG] = convert_current(data);
    bool overtemp = CURRENT_OVERTEMP * 100 < regs[MOTOR_REG];
    limited_counter(&motor_overload, overtemp, MOTOR_SHUTDOWN_THRESH);
    break;
  }

  case CS2_ADC: regs[VDD_REG] = convert_current(data); break;

  case CS3_ADC:
    regs[LOAD2_REG] = convert_current(data);
    check_load(&loads[1]);
    break;

  case CS4_ADC:
    regs[LOAD1_REG] = convert_current(data);
    check_load(&loads[0]);
    break;
  }
}


static void adc_conversion() {
  static int i = 0;

  read_conversion(ch_schedule[i]);
  if (++i == sizeof(ch_schedule)) i = 0;

  // Start next conversion
  ADMUX = (ADMUX & 0xf0) | ch_schedule[i];
  ADCSRA |= 1 << ADSC;
}


ISR(ADC_vect) {adc_conversion();}


static bool is_within(float a, float b, float tolerance) {
  return a * (1 - tolerance) < b && b < a * (1 + tolerance);
}


static void validate_input_voltage() {
  int settle = 0;
  float vlast = 0;

  while (settle < VOLTAGE_SETTLE_COUNT) {
    wdt_reset();
    delay_ms(VOLTAGE_SETTLE_PERIOD);

    float vin = get_reg(VIN_REG);

    // Check that voltage is with in range and settled
    if (VOLTAGE_MIN < vin && vin < VOLTAGE_MAX &&
        is_within(vlast, vin, VOLTAGE_SETTLE_TOLERANCE)) settle++;
    else settle = 0;

    vlast = vin;
  }
}


static void charge_caps() {
  TCCR0A |= (1 << COM0A1) | (0 << COM0A0); // Clear on compare match
  IO_PORT_CLR(MOTOR_PIN); // Motor voltage off
  IO_DDR_SET(MOTOR_PIN);  // Output

  uint64_t now = time;
  for (int i = 0; i < CAP_CHARGE_TIME; i++) {
    OCR0A = 0xff * CAP_CHARGE_MAX_DUTY / CAP_CHARGE_TIME * (i + 1);
    while (time == now) continue;
    now++;
  }

  TCCR0A &= ~((1 << COM0A1) | (1 << COM0A0));
  IO_PORT_SET(MOTOR_PIN); // Motor voltage on
}


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
  IO_PORT_CLR(MOTOR_PIN); // Motor voltage off
  IO_DDR_CLR(LOAD1_PIN);  // Tri-state
  IO_DDR_CLR(LOAD2_PIN);  // Tri-state
  IO_PUE_SET(PWR_RESET);  // Pull up reset line

  // Disable digital IO on ADC lines
  DIDR0 = (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) |
    (1 << ADC0D) | (1 << AREFD);
  DIDR1 = (1 << ADC5D);

  // ADC internal 1.1v, enable, with interrupt, prescale 128
  ADMUX = (1 << REFS1) | (0 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADIE) |
    (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRB = 0;

  // Timer 0 (Fast PWM, clk/1)
  TCCR0A = (1 << WGM01) | (1 << WGM00);
  TCCR0B = (0 << WGM02) | (0 << CS02) | (0 << CS01) | (1 << CS00);
  TIMSK = 1 << TOIE0; // Enable overflow interrupt

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


static void shutdown(uint16_t flags) {
  // Disable timers
  TCCR0B = TCCR1B = 0;

  // Disable outputs
  IO_DDR_CLR(SHUNT_PIN);  // Input
  IO_DDR_CLR(MOTOR_PIN);  // Input
  IO_PORT_CLR(LOAD1_PIN); // Lo
  IO_PORT_CLR(LOAD2_PIN); // Lo
  IO_DDR_SET(LOAD1_PIN);  // Output
  IO_DDR_SET(LOAD2_PIN);  // Output

  while (true) continue;
}


static void validate_measurements() {
  const float max_voltage = 0.99 * convert_voltage(0x3ff);
  const float max_current = 0.99 * convert_current(0x3ff);

  if (max_voltage < regs[VOUT_REG]  ||
      max_current < regs[MOTOR_REG] ||
      max_current < regs[LOAD1_REG] ||
      max_current < regs[LOAD2_REG] ||
      max_current < regs[VDD_REG])
    shutdown(MEASUREMENT_ERROR_FLAG);
}


int main() {
  wdt_enable(WDTO_8S);

  init();
  adc_conversion(); // Start ADC
  validate_input_voltage();
  charge_caps();
  validate_measurements();

  while (true) {
    wdt_reset();

    float vin = get_reg(VIN_REG);
    float vout = get_reg(VOUT_REG);

    update_shunt_power(vout, vnom);

    // Check fault conditions
    uint16_t flags = 0;
    if (vin < VOLTAGE_MIN) flags |= UNDER_VOLTAGE_FLAG;
    if (VOLTAGE_MAX < vin || VOLTAGE_MAX < vout) flags |= OVER_VOLTAGE_FLAG;
    if (CURRENT_MAX < get_total_current()) flags |= OVER_CURRENT_FLAG;
    if (shunt_overload) flags |= SHUNT_OVERLOAD_FLAG;
    if (MOTOR_SHUTDOWN_THRESH <= motor_overload) flags |= MOTOR_OVERLOAD_FLAG;
    if (loads[0].limit == LOAD_LIMIT_TICKS) flags |= LOAD1_OVERTEMP_FLAG;
    if (loads[1].limit == LOAD_LIMIT_TICKS) flags |= LOAD2_OVERTEMP_FLAG;
    if (loads[0].limit) flags |= LOAD1_LIMITING_FLAG;
    if (loads[1].limit) flags |= LOAD2_LIMITING_FLAG;

    regs[FLAGS_REG] = flags;
    if (flags & FATAL_FLAG_MASK) shutdown(flags);
  }

  return 0;
}
