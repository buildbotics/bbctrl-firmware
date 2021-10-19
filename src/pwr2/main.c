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

#include "config.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>

#include <util/delay.h>

#include <stdbool.h>
#include <math.h>


static const uint8_t crc[256] PROGMEM = {
  0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
  0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
  0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
  0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
  0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
  0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
  0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
  0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
  0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
  0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
  0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
  0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
  0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
  0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
  0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
  0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
  0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
  0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
  0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
  0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
  0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
  0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
  0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
  0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
  0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
  0x76, 0x71, 0x78, 0x7f, 0x6a, 0x6d, 0x64, 0x63,
  0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
  0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
  0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
  0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8d, 0x84, 0x83,
  0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
  0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};


uint8_t packet_error_check(uint8_t pec, uint8_t data) {
  return pgm_read_byte(&(crc[pec ^ data]));
}


typedef void (*result_cb_t)(uint16_t);


typedef struct {
  uint8_t mux;
  result_cb_t cb;
} result_t;


typedef struct {
  ADC_t *regs;
  uint8_t index;
  uint8_t count;
  result_t *results;
} adc_t;


static volatile uint16_t regs[NUM_REGS] = {0};
static volatile float shunt_joules = 0;
static volatile bool initialized = false;
static volatile float vnom = 0;
static volatile uint64_t time = 0; // ms


static void shutdown();


static void delay(uint16_t ms) {
  uint64_t end = time + ms;
  while (time < end) continue;
}


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


static void shunt_enable(bool enable) {
  if (enable) SHUNT_PORT.OUTCLR = SHUNT_PIN; // Lo
  else SHUNT_PORT.OUTSET = SHUNT_PIN;        // Hi
  SHUNT_PORT.DIRSET = SHUNT_PIN;             // Out
}


static void motor_enable(bool enable) {
  if (enable) MOTOR_PORT.OUTSET = MOTOR_PIN; // Hi
  else MOTOR_PORT.OUTCLR = MOTOR_PIN;        // Lo
  MOTOR_PORT.DIRSET = MOTOR_PIN;             // Out
}


static uint16_t to_voltage(uint16_t result) {
  return result *
    (((REFV * (VR1 + VR2)) / (VR2 * 1023 * ADC_SAMPLES)) * VSCALE * REG_SCALE);
}


static uint16_t to_current(uint16_t result) {
  return result * ((REFV / (RSENSE * 1023 * ADC_SAMPLES * ISCALE)) * REG_SCALE);
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

  float vout = regs[VOUT_REG] / REG_SCALE;

  if (vnom + SHUNT_MIN_V < vout) {
    // Compute joules shunted this cycle: J = V^2 / RT
    shunt_joules = vout * vout / (SHUNT_OHMS * 1000.0);
    shunt_enable(true);

  } else {
    shunt_joules = 0;
    shunt_enable(false);
  }
}


static void measure_nominal_voltage() {
  float vin = regs[VIN_REG] / REG_SCALE;

  if (vnom < VOLTAGE_MIN) vnom = vin;
  else vnom = vnom * (1 - VOLTAGE_EXP) + vin * VOLTAGE_EXP;
}


ISR(TCA0_OVF_vect) {
  time++;

  update_shunt(); // Every 1ms
  if (!(time & 7)) measure_nominal_voltage(); // Every 8ms

  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm; // Clear interrupt flag
}


static void vin_callback(uint16_t result) {
  uint16_t vin = regs[VIN_REG] = to_voltage(result) + (VIN_OFFSET * REG_SCALE);

  update_shunt_power();

  // Check voltage
  if (!initialized) return;
  if (vin < (VOLTAGE_MIN * REG_SCALE)) flags_set(UNDER_VOLTAGE_FLAG);
  if ((VOLTAGE_MAX * REG_SCALE) < vin) flags_set(OVER_VOLTAGE_FLAG);
}


static void vout_callback(uint16_t result) {
  uint16_t vout = regs[VOUT_REG] = to_voltage(result);

  // VOut ADC timing
  DEBUG_PORT.OUTSET = DEBUG_PIN;
  DEBUG_PORT.OUTCLR = DEBUG_PIN;

  // Check voltage
  if (!initialized) return;
  if ((VOLTAGE_MAX * REG_SCALE) < vout) flags_set(OVER_VOLTAGE_FLAG);
  flags(MOTOR_UNDER_VOLTAGE_FLAG,
        vout < (VOLTAGE_MIN * REG_SCALE) && !flags_get(POWER_SHUTDOWN_FLAG));
}


static void imon_callback(uint16_t result) {
  uint16_t cur = to_current(result);

  // Subtract off motor driver current
  const uint16_t mcur = MOTOR_DRIVER_CURRENT * REG_SCALE;
  regs[MOTOR_REG] = cur < mcur ? 0 : (cur - mcur);

  // Check current
  if (!initialized) return;
  if ((CURRENT_MAX * REG_SCALE) < cur) flags_set(MOTOR_OVERLOAD_FLAG);
}


static void null_callback(uint16_t result) {} // Ignore


static void temp_callback(uint16_t result) {
  uint8_t gain  = SIGROW.TEMPSENSE0;
  int8_t offset = SIGROW.TEMPSENSE1;
  uint32_t t    = result * (REFV / 1.1 / ADC_SAMPLES);

  static uint8_t sample = 0;
  static uint16_t samples[ADC_BUCKETS] = {0};

  samples[sample] = ((t - offset) * gain + 0x80) >> 8;
  if (++sample == ADC_BUCKETS) sample = 0;

  t = 0;
  for (int i = 0; i < ADC_BUCKETS; i++)
    t += samples[i];

  regs[TEMP_REG] = t >> ADC_SCALE;
}


static result_t adc0_results[] = {
  {VIN_ADC,  vin_callback},
  {VOUT_ADC, vout_callback},
  {TEMP_ADC, null_callback}, // Ignore first temp measure
  {TEMP_ADC, temp_callback},
};


static result_t adc1_results[] = {
  {IMON_ADC, imon_callback},
};


static adc_t adcs[] = {
  {&ADC0, 0, sizeof(adc0_results) / sizeof(result_t), adc0_results},
  {&ADC1, 0, sizeof(adc1_results) / sizeof(result_t), adc1_results},
};


static void adc_start(adc_t *adc) {
  adc->regs->MUXPOS  = adc->results[adc->index].mux;
  adc->regs->COMMAND = ADC_STCONV_bm;
}


static void adc_init(adc_t *adc) {
  adc->regs->CTRLB = ADC_SAMPNUM;
  adc->regs->CTRLC = ADC_SAMPCAP_bm | ADC_REFSEL_INTREF_gc | ADC_PRESC_DIV8_gc;
  adc->regs->INTCTRL = ADC_RESRDY_bm;
  adc->regs->CTRLA = ADC_RESSEL_10BIT_gc | ADC_ENABLE_bm | ADC_RUNSTBY_bm;
  adc->regs->SAMPCTRL = 14;
  adc->regs->CALIB = ADC_DUTYCYC_DUTY50_gc;
  adc_start(adc);
}


static void adc_measure(adc_t *adc) {
  result_t *result = &adc->results[adc->index];
  uint16_t value = adc->regs->RES;
  result->cb(value);

  // Start next conversion
  if (++adc->index == adc->count) adc->index = 0;
  adc_start(adc);
}


ISR(ADC0_RESRDY_vect) {adc_measure(&adcs[0]);}
ISR(ADC1_RESRDY_vect) {adc_measure(&adcs[1]);}


static void i2c_ack() {TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;}


static void i2c_nack() {
  TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc | TWI_ACKACT_NACK_gc;
}


static void i2c_stop() {TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;}


ISR(TWI0_TWIS_vect) {
  static uint8_t byte;
  static uint16_t reg;
  static uint8_t pec = 0;

  // Stretch clock longer to work around RPi bug
  // See https://github.com/raspberrypi/linux/issues/254
  //_delay_us(100); // Must use software delay while in interrupt

  uint8_t status = TWI0.SSTATUS;

  if (status & TWI_DIF_bm) {
    bool is_read = status & TWI_DIR_bm;

    if (!byte)
      pec = packet_error_check(pec, (I2C_ADDR << 1) + (is_read ? 1 : 0));

    if (is_read) { // Read
      if (byte < 2) {
        uint8_t data = byte++ ? reg >> 8 : reg;
        TWI0.SDATA = data;
        pec = packet_error_check(pec, data);
        i2c_ack();

      } else {
        TWI0.SDATA = pec;
        pec = 0;
        i2c_nack();
      }

    } else {       // Write
      uint8_t cmd = TWI0.SDATA;

      if (cmd < NUM_REGS) {
        reg = regs[cmd];
        pec = packet_error_check(pec, cmd);
        i2c_ack();

      } else i2c_nack();
    }

  } else if (status & TWI_APIF_bm) { // Address or stop
    if (status & TWI_AP_ADR_gc) {    // Address match
      i2c_ack();
      byte = 0;

    } else i2c_stop();
  }
}


static float get_reg(int reg) {
  uint8_t sreg = SREG;
  cli();
  float value = regs[reg];
  SREG = sreg;

  return value / REG_SCALE;
}


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


static bool discharge_wait(float min_v) {
  float vout = get_reg(VOUT_REG);
  if (vout <= min_v) return true;

  uint16_t max_ms = (vout - min_v) * MAX_DISCHARGE_WAIT_TIME + 1;

  for (uint16_t i = 0; i < max_ms; i++) {
    delay(1);
    if (get_reg(VOUT_REG) <= min_v) return true;
  }

  return false;
}


bool charge_wait(float max_v) {
  float vstart = get_reg(VOUT_REG);
  if (max_v <= vstart) return true;

  uint16_t max_ms = (max_v - vstart) * MAX_CHARGE_WAIT_TIME + 1;

  for (uint16_t i = 0; i < max_ms; i++) {
    delay(1);
    float vout = get_reg(VOUT_REG);
    if (max_v <= vout) return true;

    // Abort if not charging fast enough
    if (i && i % 16 == 0 && (vout - vstart) * MAX_CHARGE_WAIT_TIME < i)
      break;
  }

  return false;
}


static void charge_test() {
  // Wait for motor voltage to self discharge below min voltage
  if (!discharge_wait(VOLTAGE_MIN - 2)) return flags_set(GATE_ERROR_FLAG);

  // Charge caps
  shunt_enable(false);
  motor_enable(true);
  delay(GATE_TURN_ON_DELAY);
  if (!charge_wait(VOLTAGE_MIN - 1))
    return flags_set(CHARGE_ERROR_FLAG);

  // If the gate works, the caps should discharge a bit on their own
  motor_enable(false); // Motor voltage off
  if (!discharge_wait(VOLTAGE_MIN - 2)) return flags_set(GATE_ERROR_FLAG);

  // Discharge caps through shunt
  shunt_enable(true);
  delay(CAP_DISCHARGE_TIME);
  if (SHUNT_FAIL_VOLTAGE < get_reg(VOUT_REG)) flags_set(SHUNT_ERROR_FLAG);

  // Charge caps
  shunt_enable(false);
  motor_enable(true);
  charge_wait(get_reg(VIN_REG) - 2);
}


static void validate_measurements() {
  const float max_voltage = 0.99 * to_voltage(0x3ffUL * ADC_SAMPLES);
  const float max_current = 0.99 * to_current(0x3ffUL * ADC_SAMPLES);

  if (max_voltage < regs[VOUT_REG])
    flags_set(MOTOR_VOLTAGE_SENSE_ERROR_FLAG);

  if (max_current < regs[MOTOR_REG])
    flags_set(MOTOR_CURRENT_SENSE_ERROR_FLAG);

  if (flags_get(SENSE_ERROR_FLAGS))
    flags_set(SENSE_ERROR_FLAG);
}


static void shutdown() {
  if (flags_get(POWER_SHUTDOWN_FLAG)) return;
  flags_set(POWER_SHUTDOWN_FLAG);
  initialized = false;

  // Disable motors
  motor_enable(false);

  // Enable shunt
  shunt_enable(true);
}


void init() {
  cli(); // Disable Global Interrupts

  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0); // 20Mhz clock

  // Disable digital I/O on analog lines
  PORTA.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN4CTRL = PORT_ISC_INPUT_DISABLE_gc;
  PORTB.PIN5CTRL = PORT_ISC_INPUT_DISABLE_gc;

  // Select reference voltages
  VREF.CTRLA = VREF_ADC0REFSEL_4V34_gc;
  VREF.CTRLC = VREF_ADC1REFSEL_4V34_gc;

  // Configure and start ADCs
  adc_init(&adcs[0]);
  adc_init(&adcs[1]);

  // Configure I2C
  TWI0.SCTRLA    = TWI_DIEN_bm | TWI_APIEN_bm | TWI_SMEN_bm | TWI_ENABLE_bm;
  TWI0.SADDR     = I2C_ADDR << 1;
  CPUINT.LVL1VEC = TWI0_TWIS_vect_num; // High priority

  // Configure timer
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1_gc | TCA_SINGLE_ENABLE_bm;
  TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc;
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;
  TCA0.SINGLE.PER = F_CPU / 1000;

  // Debug pin
  DEBUG_PORT.OUTCLR = DEBUG_PIN;
  DEBUG_PORT.DIRSET = DEBUG_PIN;

  sei(); // Enable Global Interrupts
}


int main() {
  regs[VERSION_REG] = VERSION;

  uint16_t crc = pgm_read_word(32UL * 1024 - 2);
  regs[CRC_REG] = (crc >> 8) + (crc << 8);

  flags_set(NOT_INITIALIZED_FLAG);

  init();
  validate_input_voltage();
  charge_test();
  validate_measurements();
  initialized = true;
  flags_clear(NOT_INITIALIZED_FLAG);

  while (true) sleep_cpu();

  return 0;
}
