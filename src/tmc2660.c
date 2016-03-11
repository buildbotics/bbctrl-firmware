/******************************************************************************\

                   This file is part of the TinyG firmware.

                     Copyright (c) 2016, Buildbotics LLC
                             All rights reserved.

        The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
        You may request written permission by emailing the authors.

                For information regarding this software email:
                               Joseph Coffland
                            joseph@buildbotics.com

\******************************************************************************/

#include "tmc2660.h"
#include "status.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


void set_dcur(int driver, float value);


typedef enum {
  TMC2660_STATE_CONFIG,
  TMC2660_STATE_MONITOR,
  TMC2660_STATE_RESET,
} tmc2660_state_t;

typedef enum {
  SPI_STATE_SELECT,
  SPI_STATE_WRITE,
  SPI_STATE_READ,
  SPI_STATE_QUIT,
} spi_state_t;

typedef struct {
  tmc2660_state_t state;
  uint8_t reset;
  uint8_t reg;

  uint16_t sguard;
  uint8_t flags;
  uint32_t regs[5];
} tmc2660_driver_t;


static const uint32_t reg_addrs[] = {
  TMC2660_DRVCTRL_ADDR,
  TMC2660_CHOPCONF_ADDR,
  TMC2660_SMARTEN_ADDR,
  TMC2660_SGCSCONF_ADDR,
  TMC2660_DRVCONF_ADDR
};


static volatile uint8_t driver;
static tmc2660_driver_t drivers[TMC2660_NUM_DRIVERS];

static volatile spi_state_t spi_state;
static volatile uint8_t spi_byte;
static volatile uint32_t spi_out;
static volatile uint32_t spi_in;


static void spi_cs(int driver, int enable) {
  if (enable)
    switch (driver) {
    case 0: TMC2660_SPI_SSX_PORT.OUTCLR = 1 << TMC2660_SPI_SSX_PIN; break;
    case 1: TMC2660_SPI_SSY_PORT.OUTCLR = 1 << TMC2660_SPI_SSY_PIN; break;
    case 2: TMC2660_SPI_SSZ_PORT.OUTCLR = 1 << TMC2660_SPI_SSZ_PIN; break;
    case 3: TMC2660_SPI_SSA_PORT.OUTCLR = 1 << TMC2660_SPI_SSA_PIN; break;
    case 4: TMC2660_SPI_SSB_PORT.OUTCLR = 1 << TMC2660_SPI_SSB_PIN; break;
    }
  else
    switch (driver) {
    case 0: TMC2660_SPI_SSX_PORT.OUTSET = 1 << TMC2660_SPI_SSX_PIN; break;
    case 1: TMC2660_SPI_SSY_PORT.OUTSET = 1 << TMC2660_SPI_SSY_PIN; break;
    case 2: TMC2660_SPI_SSZ_PORT.OUTSET = 1 << TMC2660_SPI_SSZ_PIN; break;
    case 3: TMC2660_SPI_SSA_PORT.OUTSET = 1 << TMC2660_SPI_SSA_PIN; break;
    case 4: TMC2660_SPI_SSB_PORT.OUTSET = 1 << TMC2660_SPI_SSB_PIN; break;
    }
}



static void spi_send() {
  // Flush any status errors (TODO check for errors)
  uint8_t x = SPIC.STATUS;
  x = x;

  // Read
  if (!spi_byte) spi_in = 0;
  else spi_in = spi_in << 8 | SPIC.DATA;

  // Write
  if (spi_byte < 3) SPIC.DATA = 0xff & (spi_out >> ((2 - spi_byte++) * 8));
  else spi_byte = 0;
}


void spi_next() {
  tmc2660_driver_t *drv = &drivers[driver];
  uint16_t spi_delay = 1;

  switch (spi_state) {
  case SPI_STATE_SELECT:
    // Select driver
    spi_cs(driver, 1);

    // Next state
    spi_delay = 2;
    spi_state = SPI_STATE_WRITE;
    break;

  case SPI_STATE_WRITE:
    switch (drv->state) {
    case TMC2660_STATE_CONFIG:
      spi_out = reg_addrs[drv->reg] | drv->regs[drv->reg];
      break;

    case TMC2660_STATE_MONITOR:
      spi_out = reg_addrs[TMC2660_DRVCTRL] | drv->regs[TMC2660_DRVCTRL];
      break;

    case TMC2660_STATE_RESET:
      spi_out =
        reg_addrs[TMC2660_CHOPCONF] | (drv->regs[TMC2660_CHOPCONF] & 0xffff0);
      break;
    }

    spi_send();

    // Next state
    spi_delay = 4;
    spi_state = SPI_STATE_READ;
    break;

  case SPI_STATE_READ:
    // Deselect driver
    spi_cs(driver, 0);
    spi_state = SPI_STATE_SELECT;

    switch (drv->state) {
    case TMC2660_STATE_CONFIG:
      if (++drv->reg == 5) {
        drv->state = TMC2660_STATE_MONITOR;
        drv->reg = 0;
      }
      break;

    case TMC2660_STATE_MONITOR:
      // Read response
      drv->sguard = (uint16_t)((spi_in >> 14) & 0x1ff);
      drv->flags = spi_in >> 4;

      if (drv->reset) {
        drv->state = TMC2660_STATE_RESET;
        drv->reset = 0;

      } else if (++driver == TMC2660_NUM_DRIVERS) {
        driver = 0;
        spi_delay = 500;
        //spi_state = SPI_STATE_QUIT;
        break;
      }
      break;

    case TMC2660_STATE_RESET:
      drv->state = TMC2660_STATE_CONFIG;
      break;
    }

    // Next state (delay set above)
    break;

  case SPI_STATE_QUIT: break;
  }

  TMC2660_TIMER.PER = spi_delay;
}


ISR(SPIC_INT_vect) {
  spi_send();
}


ISR(TCC1_OVF_vect) {
  spi_next();
}


void tmc2660_init() {
  // Reset state
  spi_state = SPI_STATE_SELECT;
  driver = spi_byte = 0;
  memset(drivers, 0, sizeof(drivers));

  // Configure motors
  for (int i = 0; i < TMC2660_NUM_DRIVERS; i++) {
    drivers[i].state = TMC2660_STATE_CONFIG;
    drivers[i].reg = 0;

    uint32_t mstep = 0;
    switch (MOTOR_MICROSTEPS) {
    case 1: mstep = TMC2660_DRVCTRL_MRES_1; break;
    case 2: mstep = TMC2660_DRVCTRL_MRES_2; break;
    case 4: mstep = TMC2660_DRVCTRL_MRES_4; break;
    case 8: mstep = TMC2660_DRVCTRL_MRES_8; break;
    case 16: mstep = TMC2660_DRVCTRL_MRES_16; break;
    case 32: mstep = TMC2660_DRVCTRL_MRES_32; break;
    case 64: mstep = TMC2660_DRVCTRL_MRES_64; break;
    case 128: mstep = TMC2660_DRVCTRL_MRES_128; break;
    case 256: mstep = TMC2660_DRVCTRL_MRES_256; break;
    default: break; // Invalid
    }

    drivers[i].regs[TMC2660_DRVCTRL] = TMC2660_DRVCTRL_DEDGE | mstep |
      TMC2660_DRVCTRL_INTPOL;
    drivers[i].regs[TMC2660_CHOPCONF] = TMC2660_CHOPCONF_TBL_36 |
      TMC2660_CHOPCONF_HEND(3) | TMC2660_CHOPCONF_HSTART(7) |
      TMC2660_CHOPCONF_TOFF(4);
    //drivers[i].regs[TMC2660_CHOPCONF] = TMC2660_CHOPCONF_TBL_36 |
    //  TMC2660_CHOPCONF_CHM | TMC2660_CHOPCONF_HEND(7) |
    //  TMC2660_CHOPCONF_FASTD(6) | TMC2660_CHOPCONF_TOFF(7);
    drivers[i].regs[TMC2660_SMARTEN] = TMC2660_SMARTEN_SEIMIN |
      TMC2660_SMARTEN_MAX(2) | TMC2660_SMARTEN_MIN(2);
    drivers[i].regs[TMC2660_SGCSCONF] = TMC2660_SGCSCONF_SFILT |
      TMC2660_SGCSCONF_THRESH(63);
    drivers[i].regs[TMC2660_DRVCONF] = TMC2660_DRVCONF_RDSEL_SG;

    set_dcur(i, MOTOR_CURRENT);
    drivers[driver].reset = 0; // No need to reset
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

  TMC2660_SPI_SSX_PORT.OUTSET = 1 << TMC2660_SPI_SSX_PIN; // High
  TMC2660_SPI_SSX_PORT.DIRSET = 1 << TMC2660_SPI_SSX_PIN; // Output
  TMC2660_SPI_SSY_PORT.OUTSET = 1 << TMC2660_SPI_SSY_PIN; // High
  TMC2660_SPI_SSY_PORT.DIRSET = 1 << TMC2660_SPI_SSY_PIN; // Output
  TMC2660_SPI_SSZ_PORT.OUTSET = 1 << TMC2660_SPI_SSZ_PIN; // High
  TMC2660_SPI_SSZ_PORT.DIRSET = 1 << TMC2660_SPI_SSZ_PIN; // Output
  TMC2660_SPI_SSA_PORT.OUTSET = 1 << TMC2660_SPI_SSA_PIN; // High
  TMC2660_SPI_SSA_PORT.DIRSET = 1 << TMC2660_SPI_SSA_PIN; // Output
  TMC2660_SPI_SSB_PORT.OUTSET = 1 << TMC2660_SPI_SSB_PIN; // High
  TMC2660_SPI_SSB_PORT.DIRSET = 1 << TMC2660_SPI_SSB_PIN; // Output

  // Configure SPI
  PR.PRPC &= ~PR_SPI_bm; // Disable power reduction
  SPIC.CTRL = SPI_ENABLE_bm | SPI_MASTER_bm | SPI_MODE_3_gc |
    SPI_PRESCALER_DIV128_gc; // enable, big endian, master, mode 3, clock/128
  PORTC.REMAP = PORT_SPI_bm; // Swap SCK and MOSI
  SPIC.INTCTRL = SPI_INTLVL_MED_gc; // interupt level

  // Configure timer
  PR.PRPC &= ~PR_TC1_bm; // Disable power reduction
  TMC2660_TIMER.PER = F_CPU / 1024 / 10; // Set timer period
  TMC2660_TIMER.INTCTRLA = TC_OVFINTLVL_MED_gc; // overflow interupt level
  TMC2660_TIMER.CTRLA = TC_CLKSEL_DIV1024_gc; // enable, clock/1024
}


uint8_t tmc2660_flags(int driver) {
  return driver < TMC2660_NUM_DRIVERS ? drivers[driver].flags : 0;
}


void tmc2660_reset(int driver) {
  if (driver < TMC2660_NUM_DRIVERS) drivers[driver].reset = 1;
}


int tmc2660_ready(int driver) {
  return
    drivers[driver].state == TMC2660_STATE_MONITOR && !drivers[driver].reset;
}


int tmc2660_all_ready() {
  for (int i = 0; i < TMC2660_NUM_DRIVERS; i++)
    if (!tmc2660_ready(i)) return 0;

  return 1;
}


void tmc2660_get_flags(uint8_t flags, char buf[35]) {
  buf[0] = 0;

  if (TMC2660_DRVSTATUS_STST & flags) strcat(buf, "stst,");
  if (TMC2660_DRVSTATUS_OLB  & flags) strcat(buf, "olb,");
  if (TMC2660_DRVSTATUS_OLA  & flags) strcat(buf, "ola,");
  if (TMC2660_DRVSTATUS_S2GB & flags) strcat(buf, "s2gb,");
  if (TMC2660_DRVSTATUS_S2GA & flags) strcat(buf, "s2ga,");
  if (TMC2660_DRVSTATUS_OTPW & flags) strcat(buf, "otpw,");
  if (TMC2660_DRVSTATUS_OT   & flags) strcat(buf, "ot,");
  if (TMC2660_DRVSTATUS_SG   & flags) strcat(buf, "sg,");

  if (buf[0] != 0) buf[strlen(buf) - 1] = 0; // Remove last comma
}


uint8_t get_dflags(int driver) {return drivers[driver].flags;}


float get_dcur(int driver) {
  uint8_t x = drivers[driver].regs[TMC2660_SGCSCONF] & 31;
  return (x + 1) / 32.0;
}


void set_dcur(int driver, float value) {
  if (value < 0 || 1 < value) return;

  uint8_t x = value ? value * 32.0 - 1 : 0;
  if (x < 0) x = 0;

  tmc2660_driver_t *d = &drivers[driver];
  d->regs[TMC2660_SGCSCONF] = (d->regs[TMC2660_SGCSCONF] & ~31) | x;

  tmc2660_reset(driver);
}


uint16_t get_sguard(int driver) {
  return drivers[driver].sguard;
}


int8_t get_sgt(int driver) {
  uint8_t x = (drivers[driver].regs[TMC2660_SGCSCONF] & 0x7f) >> 8;
  return (x & (1 << 6)) ? (x & 0xc0) : x;
}


void set_sgt(int driver, int8_t value) {
  if (value < -64 || 63 < value) return;

  tmc2660_driver_t *d = &drivers[driver];
  d->regs[TMC2660_SGCSCONF] = (d->regs[TMC2660_SGCSCONF] & ~31) |
    TMC2660_SGCSCONF_THRESH(value);

  tmc2660_reset(driver);
}
