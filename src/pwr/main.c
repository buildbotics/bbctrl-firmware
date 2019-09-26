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
  volatile uint16_t value;
  volatile uint16_t raw;
  volatile uint16_t buckets[BUCKETS];
  volatile uint8_t index;
  volatile uint8_t fill;
  volatile uint32_t sum;
} reg_t;


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


static reg_t regs[NUM_REGS] = {{0}};
static volatile uint64_t time = 0; // ms
static volatile uint8_t motor_overload = 0;
static volatile float shunt_joules = 0;
static volatile bool initialized = false;
static volatile float vnom = 0;


void delay(uint16_t ms) {
  uint64_t end = time + ms;
  while (time < end) continue;
}


static void shutdown();


static uint16_t flags_get(uint16_t flags) {
  return regs[FLAGS_REG].value & flags;
}


static void flags_clear(uint16_t flags) {regs[FLAGS_REG].value &= ~flags;}


static void flags_set(uint16_t flags) {
  regs[FLAGS_REG].value |= flags;
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
        TWSD = byte++ ? reg >> 8 : reg;
        i2c_ack();

      } else i2c_nack();

    } else i2c_ack(); // Write ignore

  } else if (status & I2C_ADDRESS_STOP_INT_BM) {
    if (status & I2C_ADDRESS_MATCH_BM) {
      // Read address
      uint8_t addr = (TWSD >> 1) & I2C_MASK;

      if (addr < NUM_REGS) {
        i2c_ack();
        reg = regs[addr].value;
        byte = 0;

      } else i2c_nack();

    } else TWSCRB = (1 << TWCMD1) | (0 << TWCMD0);  // Stop
  }
}


static float get_reg(int reg) {
  uint8_t sreg = SREG;
  cli();
  float value = regs[reg].value;
  SREG = sreg;

  return value / REG_SCALE;
}


static void update_shunt() {
  if (!initialized) return;

  static float joules = SHUNT_JOULES; // Power disipation budget

  // Add power dissipation credit for the 1ms that elapsed
  joules += SHUNT_JOULES_PER_MS;
  if (SHUNT_JOULES < joules) joules = SHUNT_JOULES; // Max

  if (joules < shunt_joules) flags_set(SHUNT_OVERLOAD_FLAG);
  else joules -= shunt_joules; // Subtract power dissipated
}


static void update_shunt_power() {
  if (!initialized) return;

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
  float vin = regs[VIN_REG].raw / REG_SCALE;

  if (vnom < VOLTAGE_MIN) vnom = vin;
  else vnom = vnom * (1 - VOLTAGE_EXP) + vin * VOLTAGE_EXP;
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


static uint16_t average_reg(int index, uint16_t sample) {
  reg_t *reg = &regs[index];

  reg->raw = sample;
  reg->sum -= reg->buckets[reg->index];
  reg->sum += sample;
  reg->buckets[reg->index] = sample;
  if (++reg->index == BUCKETS) reg->index = 0;

  if (reg->fill < BUCKETS) {
    reg->fill++;
    reg->value = reg->sum / reg->fill;

  } else reg->value = reg->sum >> AVG_SCALE;

  return reg->value;
}


static uint16_t convert_voltage(uint16_t sample) {
  return
    sample * (VOLTAGE_REF / 1024.0 *
              (VOLTAGE_REF_R1 + VOLTAGE_REF_R2) / VOLTAGE_REF_R2 * REG_SCALE);
}


static uint16_t convert_current(uint16_t sample) {
  return sample * (VOLTAGE_REF / 1024.0 * CURRENT_REF_MUL);
}


static void update_current(int reg, uint16_t sample) {
  average_reg(reg, convert_current(sample));

  // Check total current
  if (!initialized) return;
  uint16_t total_current =
    regs[MOTOR_REG].value + regs[VDD_REG].value + regs[LOAD1_REG].value +
    regs[LOAD2_REG].value;
  if (CURRENT_MAX * REG_SCALE < total_current) flags_set(OVER_CURRENT_FLAG);
}


static void update_vin(uint16_t sample) {
  uint16_t vin = average_reg(VIN_REG, convert_voltage(sample));

  // Check voltage
  if (!initialized) return;
  if (vin < (VOLTAGE_MIN * REG_SCALE)) flags_set(UNDER_VOLTAGE_FLAG);
  if ((VOLTAGE_MAX * REG_SCALE) < vin) flags_set(OVER_VOLTAGE_FLAG);
}


static void update_vout(uint16_t sample) {
  uint16_t vout = average_reg(VOUT_REG, convert_voltage(sample));

  update_shunt_power();

  // Check voltage
  if (!initialized) return;
  if ((VOLTAGE_MAX * REG_SCALE) < vout) flags_set(OVER_VOLTAGE_FLAG);
  flags(MOTOR_UNDER_VOLTAGE_FLAG,
        vout < (VOLTAGE_MIN * REG_SCALE) && !flags_get(POWER_SHUTDOWN_FLAG));
}


static void update_motor_current(uint16_t sample) {
  update_current(MOTOR_REG, sample);

  // Check overtemp and motor overload
  if (!initialized) return;
  bool overtemp = CURRENT_OVERTEMP * REG_SCALE < regs[MOTOR_REG].value;

  if (overtemp) {
    if (motor_overload < MOTOR_SHUTDOWN_THRESH) motor_overload++;
    if (motor_overload == MOTOR_SHUTDOWN_THRESH) flags_set(MOTOR_OVERLOAD_FLAG);

  } else if (motor_overload != MOTOR_SHUTDOWN_THRESH && motor_overload)
    motor_overload--;
}


static void load_shutdown(load_t *load) {
  if (!flags_get(POWER_SHUTDOWN_FLAG)) flags_set(load->shutdown_flag);
  IO_PORT_CLR(load->pin); // Lo
  IO_DDR_SET(load->pin);  // Output
}


static void update_load_current(load_t *load, uint16_t sample) {
  update_current(load->reg, sample);

  if (!initialized || flags_get(load->shutdown_flag)) return;

  bool overtemp = CURRENT_OVERTEMP * REG_SCALE < regs[load->reg].value;

  if (overtemp) {
    if (++load->overtemp == LOAD_OVERTEMP_MAX) load_shutdown(load);
  } else if (load->overtemp) load->overtemp--;
}


static void read_conversion(uint8_t ch) {
  uint16_t sample = ADC;

  switch (ch) {
  case TEMP_ADC: regs[TEMP_REG].value = sample;          break; // in Kelvin
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
    delay(VOLTAGE_SETTLE_PERIOD);

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
  delay(CAP_CHARGE_TIME);
}


static void shunt_test() {
  charge_caps();

  // Discharge caps
  IO_PORT_CLR(MOTOR_PIN); // Motor voltage off
  IO_PORT_CLR(SHUNT_PIN); // Enable shunt (lo)
  delay(CAP_CHARGE_TIME);

  if (SHUNT_FAIL_VOLTAGE < get_reg(VOUT_REG)) flags_set(SHUNT_ERROR_FLAG);
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

  // Timer 0, normal, clk/1
  TCCR0A = (0 << WGM01) | (0 << WGM00);
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
  initialized = false;

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

  if (max_voltage < regs[VOUT_REG].value)
    flags_set(MOTOR_VOLTAGE_SENSE_ERROR_FLAG);
  if (max_current < regs[MOTOR_REG].value)
    flags_set(MOTOR_CURRENT_SENSE_ERROR_FLAG);
  if (max_current < regs[LOAD1_REG].value)
    flags_set(LOAD1_SENSE_ERROR_FLAG);
  if (max_current < regs[LOAD2_REG].value)
    flags_set(LOAD2_SENSE_ERROR_FLAG);
  if (max_current < regs[VDD_REG].value)
    flags_set(VDD_CURRENT_SENSE_ERROR_FLAG);
  if (flags_get(SENSE_ERROR_FLAGS))  flags_set(SENSE_ERROR_FLAG);
}


int main() {
  regs[VERSION_REG].value = VERSION;

  init();
  adc_conversion(); // Start ADC
  validate_input_voltage();
  shunt_test();
  charge_caps();
  validate_measurements();
  initialized = true;

  while (true) continue;

  return 0;
}
