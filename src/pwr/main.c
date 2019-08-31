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

#include <util/delay.h>

#include <stdbool.h>


typedef struct {
  const regs_t reg;
  const uint8_t pin;
  volatile uint8_t overtemp;
  volatile uint16_t shutdown_flag;
} load_t;


load_t loads[2] = {
  {LOAD1_REG, LOAD1_PIN, 0, LOAD1_SHUTDOWN_FLAG},
  {LOAD2_REG, LOAD2_PIN, 0, LOAD2_SHUTDOWN_FLAG},
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
static volatile float shunt_joules = 0;
static volatile float vnom = 0;


static void shutdown();


static uint16_t flags_get(uint16_t flags) {return regs[FLAGS_REG] & flags;}
static void flags_clear(uint16_t flags) {regs[FLAGS_REG] &= ~flags;}


static void flags_set(uint16_t flags) {
  regs[FLAGS_REG] |= flags;
  if (flags & FATAL_FLAGS) shutdown();
}


static void flags(uint16_t flags, bool enable) {
  if (enable) flags_set(flags);
  else flags_clear(flags);
}


static void i2c_ack()  {TWSCRB = (1 << TWCMD1) | (1 << TWCMD0);}
static void i2c_nack() {TWSCRB = (1 << TWAA) | (1 << TWCMD1) | (1 << TWCMD0);}


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


static float get_reg(int reg) {
  cli();
  float value = regs[reg];
  sei();

  return value / 100;
}


static void update_shunt() {
  if (flags_get(POWER_SHUTDOWN_FLAG)) return;

  static float joules = SHUNT_JOULES; // Power disipation budget

  // Add power dissipation credit for the 1ms that elapsed
  joules += SHUNT_JOULES_PER_MS;
  if (SHUNT_JOULES < joules) joules = SHUNT_JOULES; // Max

  if (joules < shunt_joules) flags_set(SHUNT_OVERLOAD_FLAG);
  else joules -= shunt_joules; // Subtract power dissipated
}


static void update_shunt_power() {
  if (flags_get(POWER_SHUTDOWN_FLAG)) return;

  float vout = get_reg(VOUT_REG);

  if (vnom + SHUNT_MIN_V < vout) {
    // Compute joules shunted this cycle: J = V^2 / RT
    shunt_joules = vout * vout / (SHUNT_OHMS * 1000.0);
    IO_PORT_CLR(SHUNT_PIN); // Enable (lo)

  } else {
    shunt_joules = 0;
    IO_PORT_SET(SHUNT_PIN); // Disable (hi)
  }
}


static void measure_nominal_voltage() {
  float vin = get_reg(VIN_REG);
  float v;

  if (vnom < VOLTAGE_MIN) v = vin;
  else v = vnom * (1 - VOLTAGE_EXP) + vin * VOLTAGE_EXP;

  vnom = v;
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


static uint16_t convert_voltage(uint16_t sample) {
  return sample * (VOLTAGE_REF / 1024.0 *
                   (VOLTAGE_REF_R1 + VOLTAGE_REF_R2) / VOLTAGE_REF_R2 * 100);
}


static uint16_t convert_current(uint16_t sample) {
  return sample * (VOLTAGE_REF / 1024.0 * CURRENT_REF_MUL);
}


static void update_current(int reg, uint16_t sample) {
  reg_avg[reg][reg_index[reg]] = convert_current(sample);
  if (++reg_index[reg] == BUCKETS) reg_index[reg] = 0;

  uint32_t sum = 0;
  for (int i = 0; i < BUCKETS; i++)
    sum += reg_avg[reg][i];

  regs[reg] = sum >> AVG_SCALE;

  // Check total current
  uint16_t total_current =
    regs[MOTOR_REG] + regs[VDD_REG] + regs[LOAD1_REG] + regs[LOAD2_REG];
  if (CURRENT_MAX * 100 < total_current) flags_set(OVER_CURRENT_FLAG);
}


static void update_vin(uint16_t sample) {
  uint16_t vin = regs[VIN_REG] = convert_voltage(sample);

  // Check voltage
  if (vin < (VOLTAGE_MIN * 100)) flags_set(UNDER_VOLTAGE_FLAG);
  if ((VOLTAGE_MAX * 100) < vin) flags_set(OVER_VOLTAGE_FLAG);
}


static void update_vout(uint16_t sample) {
  uint16_t vout = regs[VOUT_REG] = convert_voltage(sample);

  update_shunt_power();

  // Check voltage
  if ((VOLTAGE_MAX * 100) < vout) flags_set(OVER_VOLTAGE_FLAG);
  flags(MOTOR_UNDER_VOLTAGE_FLAG,
        vout < (VOLTAGE_MIN * 100) && !flags_get(POWER_SHUTDOWN_FLAG));
}


static void update_motor_current(uint16_t sample) {
  update_current(MOTOR_REG, sample);

  if (flags_get(MOTOR_OVERLOAD_FLAG)) return;

  bool overtemp = CURRENT_OVERTEMP * 100 < regs[MOTOR_REG];

  if (overtemp) {
    if (motor_overload < MOTOR_SHUTDOWN_THRESH) motor_overload++;
    if (motor_overload == MOTOR_SHUTDOWN_THRESH) flags_set(MOTOR_OVERLOAD_FLAG);

  } else if (motor_overload != MOTOR_SHUTDOWN_THRESH && motor_overload)
    motor_overload--;
}


static void load_shutdown(load_t *load) {
  flags_set(load->shutdown_flag);
  IO_PORT_CLR(load->pin); // Lo
  IO_DDR_SET(load->pin);  // Output
}


static void update_load_current(load_t *load, uint16_t sample) {
  update_current(load->reg, sample);

  if (flags_get(load->shutdown_flag)) return;

  bool overtemp = CURRENT_OVERTEMP * 100 < regs[load->reg];

  if (overtemp) {
    if (++load->overtemp == LOAD_OVERTEMP_MAX) load_shutdown(load);
  } else if (load->overtemp) load->overtemp--;
}


static void read_conversion(uint8_t ch) {
  uint16_t sample = ADC;

  switch (ch) {
  case TEMP_ADC: regs[TEMP_REG] = sample;                break; // in Kelvin
  case VIN_ADC:  update_vin(sample);                     break;
  case VOUT_ADC: update_vout(sample);                    break;
  case CS1_ADC:  update_motor_current(sample);           break;
  case CS2_ADC:  update_current(VDD_REG, sample);        break;
  case CS3_ADC:  update_load_current(&loads[1], sample); break;
  case CS4_ADC:  update_load_current(&loads[0], sample); break;
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
    _delay_ms(VOLTAGE_SETTLE_PERIOD);

    // Check that voltage is with in range and settled
    float vin = get_reg(VIN_REG);
    if (VOLTAGE_MIN < vin && vin < VOLTAGE_MAX &&
        is_within(vlast, vin, VOLTAGE_SETTLE_TOLERANCE)) settle++;
    else settle = 0;

    vlast = vin;
  }
}


static void charge_caps() {
  IO_PORT_SET(SHUNT_PIN); // Disable shunt (hi)
  IO_PORT_SET(MOTOR_PIN); // Motor voltage on
  _delay_ms(CAP_CHARGE_TIME);
}


void init() {
  cli();

  // CPU Clock, disable CKOUT
  CCP   = 0xd8;
  CLKSR = (1 << CSTR) | (1 << CKOUT_IO) | 0b0010; // 8Mhz internal clock
  CCP   = 0xd8;
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
  IO_PORT_CLR(SHUNT_PIN); // Enable shunt
  IO_DDR_SET(SHUNT_PIN);  // Output

  // Disable digital IO on ADC lines
  DIDR0 = (1 << ADC4D) | (1 << ADC3D) | (1 << ADC2D) | (1 << ADC1D) |
    (1 << ADC0D) | (1 << AREFD);
  DIDR1 = (1 << ADC5D);

  // ADC internal 1.1v, enable, with interrupt, prescale 64
  // Note, a conversion takes ~200uS
  ADMUX  = (1 << REFS1) | (0 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADIE) |
    (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0);
  ADCSRB = 0;

  // Timer 0 (Fast PWM, clk/1)
  TCCR0A = (1 << WGM01) | (1 << WGM00);
  TCCR0B = (0 << WGM02) | (0 << CS02) | (0 << CS01) | (1 << CS00);
  TIMSK  = 1 << TOIE0; // Enable overflow interrupt

  // I2C, enable, enable address/stop interrupt
  TWSCRA = (1 << TWEN) | (1 << TWASIE) | (1 << TWDIE);
  TWSA   = I2C_ADDR << 1;
  TWSAM  = I2C_MASK << 1;

  sei();
}


static void shutdown() {
  if (flags_get(POWER_SHUTDOWN_FLAG)) return;
  flags_set(POWER_SHUTDOWN_FLAG);

  // Disable loads
  load_shutdown(&loads[0]);
  load_shutdown(&loads[1]);

  // Motor power off
  IO_PORT_CLR(MOTOR_PIN); // Lo

  // Turn shunt on
  IO_PORT_CLR(SHUNT_PIN); // Lo
}


static void validate_measurements() {
  const float max_voltage = 0.99 * convert_voltage(0x3ff);
  const float max_current = 0.99 * convert_current(0x3ff);

  if (max_voltage < regs[VOUT_REG])  flags_set(MOTOR_VOLTAGE_SENSE_ERROR_FLAG);
  if (max_current < regs[MOTOR_REG]) flags_set(MOTOR_CURRENT_SENSE_ERROR_FLAG);
  if (max_current < regs[LOAD1_REG]) flags_set(LOAD1_SENSE_ERROR_FLAG);
  if (max_current < regs[LOAD2_REG]) flags_set(LOAD2_SENSE_ERROR_FLAG);
  if (max_current < regs[VDD_REG])   flags_set(VDD_CURRENT_SENSE_ERROR_FLAG);
  if (flags_get(SENSE_ERROR_FLAGS))  flags_set(SENSE_ERROR_FLAG);
}


int main() {
  regs[VERSION_REG] = VERSION;

  init();
  adc_conversion(); // Start ADC
  validate_input_voltage();
  charge_caps();
  validate_measurements();

  while (true) continue;

  return 0;
}
