/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
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

#include "tmc2660.h"
#include "status.h"
#include "motor.h"
#include "rtc.h"
#include "cpp_magic.h"
#include "canonical_machine.h"
#include "plan/calibrate.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#define REGS 5


typedef struct {
  bool wrote_data;
  bool configured;
  bool monitor;
  uint8_t reg;
  uint32_t stabilizing;

  uint16_t sguard;
  uint8_t flags;
  uint32_t last_regs[REGS];
  uint32_t regs[REGS];

  float idle_current;
  float drive_current;

  PORT_t *port;
} tmc2660_driver_t;


static const uint32_t reg_addrs[] = {
  TMC2660_DRVCTRL_ADDR,
  TMC2660_CHOPCONF_ADDR,
  TMC2660_SMARTEN_ADDR,
  TMC2660_SGCSCONF_ADDR,
  TMC2660_DRVCONF_ADDR,
};


static tmc2660_driver_t drivers[MOTORS] = {
  {.port = &PORT_MOTOR_1},
  {.port = &PORT_MOTOR_2},
  {.port = &PORT_MOTOR_3},
  {.port = &PORT_MOTOR_4},
};


typedef struct {
  volatile uint8_t driver;
  volatile bool read;
  volatile uint8_t byte;
  volatile uint32_t out;
  volatile uint32_t in;
} spi_t;

static spi_t spi = {};



static void _report_error_flags(int driver) {
  tmc2660_driver_t *drv = &drivers[driver];

  if (drv->stabilizing < rtc_get_time()) return;

  uint8_t dflags = drv->flags;
  uint8_t mflags = 0;

  if ((TMC2660_DRVSTATUS_SHORT_TO_GND_A | TMC2660_DRVSTATUS_SHORT_TO_GND_B) &
      dflags) mflags |= MOTOR_FLAG_SHORTED_bm;

  if (TMC2660_DRVSTATUS_OVERTEMP_WARN & dflags)
    mflags |= MOTOR_FLAG_OVERTEMP_WARN_bm;

  if (TMC2660_DRVSTATUS_OVERTEMP & dflags) mflags |= MOTOR_FLAG_OVERTEMP_bm;

  if (drv->port->IN & FAULT_BIT_bm) mflags |= MOTOR_FLAG_STALLED_bm;

  if (mflags) motor_error_callback(driver, mflags);
}


static void _spi_cs(int driver, bool enable) {
  if (enable) drivers[driver].port->OUTCLR = CHIP_SELECT_BIT_bm;
  else drivers[driver].port->OUTSET = CHIP_SELECT_BIT_bm;
}


static void _spi_next();


static void _spi_send() {
  // Flush any status errors (TODO check for errors)
  uint8_t x = SPIC.STATUS;
  x = x;

  // Read
  if (!spi.byte) spi.in = 0;
  else spi.in = spi.in << 8 | SPIC.DATA;

  // Write
  if (spi.byte < 3) SPIC.DATA = 0xff & (spi.out >> ((2 - spi.byte++) * 8));
  else {
    // SPI transfer complete
    spi.byte = 0;
    _spi_next();
  }
}


static void _driver_write(int driver) {
  tmc2660_driver_t *drv = &drivers[driver];

  _spi_cs(spi.driver, true);  // Select driver

  if (drv->monitor) spi.out = TMC2660_DRVCTRL_ADDR | drv->regs[TMC2660_DRVCTRL];
  else spi.out = reg_addrs[drv->reg] | drv->regs[drv->reg];

  drv->wrote_data = true;
  _spi_send(); // Start transfer
}


static bool _driver_read(int driver) {
  tmc2660_driver_t *drv = &drivers[driver];

  _spi_cs(spi.driver, false); // Deselect driver

  // Read response
  bool read_data = drv->wrote_data;
  if (drv->wrote_data) {
    drv->wrote_data = false;

    // Read bits [23, 4]
    drv->sguard = (spi.in >> 14) & 0x3ff;
    drv->flags = spi.in >> 4;

    calibrate_set_stallguard(driver, drv->sguard);

    // Write driver 0 stallguard to DAC
    //if (driver == 0 && (DACB.STATUS & DAC_CH0DRE_bm))
    //  DACB.CH0DATA = drv->sguard << 2;

    _report_error_flags(driver);
  }

  // Check if regs have changed
  for (int i = 0; i < REGS; i++)
    if (drv->last_regs[i] != drv->regs[i]) {
      // Reg changed, update driver
      drv->monitor = false;
      drv->last_regs[drv->reg] = drv->regs[drv->reg];
      drv->stabilizing = rtc_get_time() + TMC2660_STABILIZE_TIME * 1000;
      drv->configured = false;
      return true;

    } else if (++drv->reg == REGS) drv->reg = 0;

  if (!drv->configured && drv->stabilizing < rtc_get_time()) {
    motor_driver_callback(driver);
    drv->configured = true;

    // Enable motor when first fully configured
    drv->port->OUTCLR = MOTOR_ENABLE_BIT_bm;
  }

  if (!drv->monitor || !read_data) {
    drv->monitor = true;
    return true;
  }

  return false;
}


static void _spi_next() {
  bool hasMore = _driver_read(spi.driver);

  if (!hasMore && ++spi.driver == MOTORS) {
    spi.driver = 0;
    TMC2660_TIMER.CTRLA = TMC2660_TIMER_ENABLE;
    return;
  }

  _driver_write(spi.driver); // Write
}


ISR(SPIC_INT_vect) {
  _spi_send();
}


ISR(TCC1_OVF_vect) {
  TMC2660_TIMER.CTRLA = 0; // Disable clock
  _spi_next();
}


void _fault_isr(int motor) {
  if (drivers[motor].stabilizing < rtc_get_time())
    motor_error_callback(motor, MOTOR_FLAG_STALLED_bm);
}


ISR(PORT_1_FAULT_ISR_vect) {_fault_isr(0);}
ISR(PORT_2_FAULT_ISR_vect) {_fault_isr(1);}
ISR(PORT_3_FAULT_ISR_vect) {_fault_isr(2);}
ISR(PORT_4_FAULT_ISR_vect) {_fault_isr(3);}


void tmc2660_init() {
  // Configure motors
  for (int i = 0; i < MOTORS; i++) {
    for (int j = 0; j < REGS; j++)
      drivers[i].last_regs[j] = -1; // Force config

    drivers[i].idle_current = MOTOR_IDLE_CURRENT;
    drivers[i].drive_current = MOTOR_CURRENT;

    uint32_t mstep = 0;
    switch (MOTOR_MICROSTEPS) {
    case 1:   mstep = TMC2660_DRVCTRL_MRES_1;   break;
    case 2:   mstep = TMC2660_DRVCTRL_MRES_2;   break;
    case 4:   mstep = TMC2660_DRVCTRL_MRES_4;   break;
    case 8:   mstep = TMC2660_DRVCTRL_MRES_8;   break;
    case 16:  mstep = TMC2660_DRVCTRL_MRES_16;  break;
    case 32:  mstep = TMC2660_DRVCTRL_MRES_32;  break;
    case 64:  mstep = TMC2660_DRVCTRL_MRES_64;  break;
    case 128: mstep = TMC2660_DRVCTRL_MRES_128; break;
    case 256: mstep = TMC2660_DRVCTRL_MRES_256; break;
    default: break; // Invalid
    }

    drivers[i].regs[TMC2660_DRVCTRL] = TMC2660_DRVCTRL_DEDGE | mstep |
      (MOTOR_MICROSTEPS == 16 ? TMC2660_DRVCTRL_INTPOL : 0);

    drivers[i].regs[TMC2660_CHOPCONF] = TMC2660_CHOPCONF_TBL_16 |
      TMC2660_CHOPCONF_HEND(3) | TMC2660_CHOPCONF_HSTART(7) |
      TMC2660_CHOPCONF_TOFF(4);
    //drivers[i].regs[TMC2660_CHOPCONF] = TMC2660_CHOPCONF_TBL_36 |
    //  TMC2660_CHOPCONF_CHM | TMC2660_CHOPCONF_HEND(7) |
    //  TMC2660_CHOPCONF_FASTD(6) | TMC2660_CHOPCONF_TOFF(7);

    drivers[i].regs[TMC2660_SMARTEN] = TMC2660_SMARTEN_SEIMIN |
      TMC2660_SMARTEN_SE(350, 450);
    drivers[i].regs[TMC2660_SMARTEN] = 0; // Disable CoolStep

    drivers[i].regs[TMC2660_SGCSCONF] = TMC2660_SGCSCONF_SFILT |
      TMC2660_SGCSCONF_THRESH(63);

    drivers[i].regs[TMC2660_DRVCONF] = TMC2660_DRVCONF_RDSEL_SG;

    tmc2660_disable(i);
  }

  // Setup pins
  // Must set the SS pin either in/high or any/output for master mode to work
  TMC2660_SPI_PORT.OUTSET = 1 << TMC2660_SPI_SS_PIN;   // High
  TMC2660_SPI_PORT.DIRSET = 1 << TMC2660_SPI_SS_PIN;   // Output

  TMC2660_SPI_PORT.OUTSET = 1 << TMC2660_SPI_SCK_PIN;  // High
  TMC2660_SPI_PORT.DIRSET = 1 << TMC2660_SPI_SCK_PIN;  // Output

  TMC2660_SPI_PORT.DIRCLR = 1 << TMC2660_SPI_MISO_PIN; // Input
  TMC2660_SPI_PORT.OUTSET = 1 << TMC2660_SPI_MOSI_PIN; // High
  TMC2660_SPI_PORT.DIRSET = 1 << TMC2660_SPI_MOSI_PIN; // Output

  for (int driver = 0; driver < MOTORS; driver++) {
    PORT_t *port = drivers[driver].port;

    port->OUTSET = CHIP_SELECT_BIT_bm;  // High
    port->OUTSET = MOTOR_ENABLE_BIT_bm; // High (disabled)
    port->DIR = MOTOR_PORT_DIR_gm;      // Pin directions

    port->PIN4CTRL = PORT_ISC_RISING_gc;
    port->INT1MASK = FAULT_BIT_bm;        // INT1
    port->INTCTRL |= PORT_INT1LVL_HI_gc;
  }

  // Configure SPI
  PR.PRPC &= ~PR_SPI_bm; // Disable power reduction
  SPIC.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_3_gc | SPI_CLK2X_bm |
    SPI_PRESCALER_DIV16_gc; // enable, big endian, master, mode 3, clock/8
  PORTC.REMAP = PORT_SPI_bm; // Swap SCK and MOSI
  SPIC.INTCTRL = SPI_INTLVL_LO_gc; // interupt level

  // Configure timer
  PR.PRPC &= ~PR_TC1_bm; // Disable power reduction
  TMC2660_TIMER.PER = F_CPU / 64 * TMC2660_POLL_RATE;
  TMC2660_TIMER.INTCTRLA = TC_OVFINTLVL_LO_gc;  // overflow interupt level
  TMC2660_TIMER.CTRLA = TMC2660_TIMER_ENABLE;

  // Configure DAC channel 0 for output
  //DACB.CTRLA = DAC_CH0EN_bm | DAC_ENABLE_bm;
  //DACB.CTRLB = DAC_CHSEL_SINGLE_gc;
  //DACB.CTRLC = DAC_REFSEL_AVCC_gc;
}


static void _set_reg(int motor, int reg, uint32_t value) {
  if (drivers[motor].regs[reg] == value) return;

  drivers[motor].regs[reg] = value;
  drivers[motor].configured = false;
}


static uint32_t _get_reg(int motor, int reg) {
  return drivers[motor].regs[reg];
}


static float _get_current(int motor) {
  return (_get_reg(motor, TMC2660_SGCSCONF) & 31) / 31.0;
}


static void _set_current(int motor, float value) {
  if (value < 0 || 1 < value) return;

  uint32_t reg =
    (_get_reg(motor, TMC2660_SGCSCONF) & ~31) | (uint8_t)(value * 31.0);
  _set_reg(motor, TMC2660_SGCSCONF, reg);
}


uint8_t tmc2660_flags(int motor) {
  return motor < MOTORS ? drivers[motor].flags : 0;
}


bool tmc2660_ready(int motor) {
  return drivers[motor].configured;
}


stat_t tmc2660_sync() {
  for (int i = 0; i < MOTORS; i++)
    if (!tmc2660_ready(i)) return STAT_EAGAIN;

  return STAT_OK;
}


void tmc2660_enable(int driver) {
  _set_current(driver, drivers[driver].drive_current);
}


void tmc2660_disable(int driver) {
  _set_current(driver, drivers[driver].idle_current);
}


void tmc2660_set_stallguard_threshold(int driver, int8_t threshold) {
  drivers[driver].regs[TMC2660_SGCSCONF] =
    (drivers[driver].regs[TMC2660_SGCSCONF] & ~TMC2660_SGCSCONF_THRESH_bm) |
    TMC2660_SGCSCONF_THRESH(threshold);
}


float get_power_level(int driver) {
  return drivers[driver].drive_current;
}


void set_power_level(int driver, float value) {
  drivers[driver].drive_current = value;
}


float get_idle_level(int driver) {
  return drivers[driver].idle_current;
}


void set_idle_level(int driver, float value) {
  drivers[driver].idle_current = value;
}


float get_current_level(int driver) {
  return _get_current(driver);
}


void set_current_level(int driver, float value) {
  _set_current(driver, value);
}


uint16_t get_sg_value(int driver) {
  return drivers[driver].sguard;
}


int8_t get_stallguard(int driver) {
  uint8_t x = (_get_reg(driver, TMC2660_SGCSCONF) & 0x7f00) >> 8;
  return (x & (1 << 6)) ? (x & 0xc0) : x;
}


void set_stallguard(int driver, int8_t value) {
  if (value < -64 || 63 < value) return;

  _set_reg(driver, TMC2660_SGCSCONF,
           (_get_reg(driver, TMC2660_SGCSCONF) & ~0x7f00) |
           TMC2660_SGCSCONF_THRESH(value));
}
