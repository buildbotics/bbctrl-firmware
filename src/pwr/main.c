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
  volatile uint8_t overtemp;
  volatile bool shutdown;
} load_t;


load_t loads[2] = {
  {LOAD1_REG, LOAD1_PIN, 0, false},
  {LOAD2_REG, LOAD2_PIN, 0, false},
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
static volatile uint16_t reg_avg[NUM_REGS][BUCKETS] = {{0}};
static volatile uint16_t reg_index[NUM_REGS] = {0};
static volatile uint64_t time = 0; // ms
static volatile uint8_t motor_overload = 0;
static volatile bool shunt_overload = false;
static volatile float shunt_joules = 0;
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
  static float joules = SHUNT_JOULES; // Power disipation budget

  // Add power dissipation credit for the 1ms that elapsed
  joules += SHUNT_JOULES_PER_MS;
  if (SHUNT_JOULES < joules) joules = SHUNT_JOULES; // Max

  if (joules < shunt_joules) shunt_overload = true;
  else if (shunt_joules) {
    joules -= shunt_joules; // Subtract power dissipated
    IO_DDR_SET(SHUNT_PIN);  // Enable
    return;
  }

  IO_DDR_CLR(SHUNT_PIN); // Disable
}


static void update_shunt_power(float vout, float vnom) {
  if (vnom + SHUNT_MIN_V < vout) {
    float duty = (vout - vnom - SHUNT_MIN_V) / SHUNT_MAX_V;
    if (duty < 0) duty = 0;
    if (1 < duty) duty = 1;
    if (VOLTAGE_MAX <= vout) duty = 1; // Full shunt at max voltage

    // Compute joules shunted this cycle: J = V^2 / RT
    shunt_joules = duty * vout * vout / (SHUNT_OHMS * 1000.0);
    OCR1A = 0xff * duty;

  } else shunt_joules = 0;
}


static void measure_nominal_voltage() {
  float vin = get_reg(VIN_REG);
  float v;

  if (vnom < VOLTAGE_MIN) v = vin;
  else v = vnom * (1 - VOLTAGE_EXP) + vin * VOLTAGE_EXP;

  vnom = v;
}


static void check_load(load_t *load) {
  if (load->shutdown) return;

  bool overtemp = CURRENT_OVERTEMP * 100 < regs[load->reg];
  if (overtemp) {
    if (++load->overtemp == LOAD_OVERTEMP_MAX) {
      load->shutdown = true;
      IO_PORT_CLR(load->pin); // Lo
      IO_DDR_SET(load->pin);  // Output
    }
  } else if (load->overtemp) load->overtemp--;
}


ISR(TIMER0_OVF_vect) {
  static uint8_t tick = 0;

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


inline static void update_current(int reg, uint16_t sample) {
  reg_avg[reg][reg_index[reg]] = convert_current(sample);
  if (++reg_index[reg] == BUCKETS) reg_index[reg] = 0;

  uint32_t sum = 0;
  for (int i = 0; i < BUCKETS; i++)
    sum += reg_avg[reg][i];

  regs[reg] = sum >> AVG_SCALE;
}


static void read_conversion(uint8_t ch) {
  uint16_t data = ADC;

  switch (ch) {
  case TEMP_ADC: regs[TEMP_REG] = data; break; // in Kelvin
  case VIN_ADC:  regs[VIN_REG]  = convert_voltage(data); break;
  case VOUT_ADC: regs[VOUT_REG] = convert_voltage(data); break;

  case CS1_ADC: {
    uint16_t raw = convert_current(data);
    bool overtemp = CURRENT_OVERTEMP * 10 < raw;

    if (overtemp) {
      if (motor_overload < MOTOR_SHUTDOWN_THRESH) motor_overload++;

    } else {
      if (motor_overload != MOTOR_SHUTDOWN_THRESH && motor_overload)
        motor_overload--;

      update_current(MOTOR_REG, data);
    }
    break;
  }

  case CS2_ADC: update_current(VDD_REG, data); break;

  case CS3_ADC:
    update_current(LOAD2_REG, data);
    check_load(&loads[1]);
    break;

  case CS4_ADC:
    update_current(LOAD1_REG, data);
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
  uint64_t now = time;

  IO_PORT_SET(MOTOR_PIN); // Motor voltage on
  while (time < now + CAP_CHARGE_TIME) continue;
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
  IO_DDR_SET(MOTOR_PIN);  // Output
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
  OCR1A = 0; // Shunt off
  TCCR1A = (1 << COM1A1) | (1 << COM1A0) | (0 << WGM11) | (1 << WGM10);
  TCCR1B =
    (0 << WGM13) | (1 << WGM12) | (0 << CS12) | (0 << CS11) | (1 << CS10);

  // I2C, enable, enable address/stop interrupt
  TWSCRA = (1 << TWEN) | (1 << TWASIE) | (1 << TWDIE);
  TWSA = I2C_ADDR << 1;
  TWSAM = I2C_MASK << 1;

  sei();
}


static void shutdown() {
  // Disable outputs
  IO_DDR_CLR(MOTOR_PIN);  // Input
  IO_PORT_CLR(LOAD1_PIN); // Lo
  IO_PORT_CLR(LOAD2_PIN); // Lo
  IO_DDR_SET(LOAD1_PIN);  // Output
  IO_DDR_SET(LOAD2_PIN);  // Output
}


static uint16_t validate_measurements() {
  const float max_voltage = 0.99 * convert_voltage(0x3ff);
  const float max_current = 0.99 * convert_current(0x3ff);
  uint16_t flags = 0;

  if (max_voltage < regs[VOUT_REG])  flags |= MOTOR_VOLTAGE_SENSE_ERROR_FLAG;
  if (max_current < regs[MOTOR_REG]) flags |= MOTOR_CURRENT_SENSE_ERROR_FLAG;
  if (max_current < regs[LOAD1_REG]) flags |= LOAD1_SENSE_ERROR_FLAG;
  if (max_current < regs[LOAD2_REG]) flags |= LOAD2_SENSE_ERROR_FLAG;
  if (max_current < regs[VDD_REG])   flags |= VDD_CURRENT_SENSE_ERROR_FLAG;

  return flags ? SENSE_ERROR_FLAG | flags : 0;
}


int main() {
  wdt_enable(WDTO_8S);

  regs[VERSION_REG] = VERSION;

  init();
  adc_conversion(); // Start ADC
  validate_input_voltage();
  charge_caps();
  uint16_t fatal = validate_measurements();

  while (true) {
    wdt_reset();

    float vin = get_reg(VIN_REG);
    float vout = get_reg(VOUT_REG);

    update_shunt_power(vout, vnom);

    // Fatal conditions
    if (vin < VOLTAGE_MIN) fatal |= UNDER_VOLTAGE_FLAG;
    if (VOLTAGE_MAX < vin || VOLTAGE_MAX < vout) fatal |= OVER_VOLTAGE_FLAG;
    if (CURRENT_MAX < get_total_current()) fatal |= OVER_CURRENT_FLAG;
    if (shunt_overload) fatal |= SHUNT_OVERLOAD_FLAG;
    if (motor_overload == MOTOR_SHUTDOWN_THRESH) fatal |= MOTOR_OVERLOAD_FLAG;
    if (fatal) shutdown();

    // Nonfatal conditions
    uint16_t nonfatal = 0;
    if (loads[0].shutdown) nonfatal |= LOAD1_SHUTDOWN_FLAG;
    if (loads[1].shutdown) nonfatal |= LOAD2_SHUTDOWN_FLAG;
    if (vout < VOLTAGE_MIN) nonfatal |= MOTOR_UNDER_VOLTAGE_FLAG;

    // Update flags
    regs[FLAGS_REG] = fatal | nonfatal;
  }

  return 0;
}
