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

#include "tinyg.h"
#include "config.h"
#include "tmc2660.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>


typedef enum {
  TMC2660_STATE_CONFIG,
  TMC2660_STATE_MONITOR,
  TMC2660_STATE_RESET,
} tmc2660_state_t;

typedef enum {
  SPI_STATE_SELECT,
  SPI_STATE_WRITE,
  SPI_STATE_READ,
} spi_state_t;

typedef struct {
  tmc2660_state_t state;
  uint8_t reset;
  uint8_t reg;

  int16_t mstep;
  uint8_t status;
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
      spi_out = reg_addrs[TMC2660_DRVCONF] | drv->regs[TMC2660_DRVCONF];
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

    switch (drv->state) {
    case TMC2660_STATE_CONFIG:
      if (++drv->reg == 5) {
        drv->state = TMC2660_STATE_MONITOR;
        drv->reg = 0;
      }
      break;

    case TMC2660_STATE_MONITOR:
      // Read response
      drv->mstep = (int16_t)((spi_in >> 14) & 0x1ff);
      if (spi_in & (1UL << 19)) drv->mstep = -drv->mstep;
      drv->status = spi_in >> 4;

      if (drv->reset) {
        drv->state = TMC2660_STATE_RESET;
        drv->reset = 0;

      } else if (++driver == TMC2660_NUM_DRIVERS) {
        driver = 0;
        spi_delay = 500;
        break;
      }
      break;

    case TMC2660_STATE_RESET:
      drv->state = TMC2660_STATE_CONFIG;
      break;
    }

    // Next state (delay set above)
    spi_state = SPI_STATE_SELECT;
    break;
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

    drivers[i].regs[TMC2660_DRVCTRL] = TMC2660_DRVCTRL_DEDGE | TMC2660_DRVCTRL_MRES_8;
    drivers[i].regs[TMC2660_CHOPCONF] = TMC2660_CHOPCONF_TBL_36 |
      TMC2660_CHOPCONF_HEND(0) | TMC2660_CHOPCONF_HSTART(4) |
      TMC2660_CHOPCONF_TOFF(4);
    //drivers[i].regs[TMC2660_CHOPCONF] = TMC2660_CHOPCONF_TBL_36 |
    //  TMC2660_CHOPCONF_CHM | TMC2660_CHOPCONF_HEND(7) |
    //  TMC2660_CHOPCONF_HSTART(6) | TMC2660_CHOPCONF_TOFF(7);
    //drivers[i].regs[TMC2660_SMARTEN] = TMC2660_SMARTEN_SEIMIN |
    //  TMC2660_SMARTEN_MAX(2) | TMC2660_SMARTEN_MIN(2);
    drivers[i].regs[TMC2660_SGCSCONF] = TMC2660_SGCSCONF_SFILT |
      TMC2660_SGCSCONF_THRESH(63) | TMC2660_SGCSCONF_CS(6);
    drivers[i].regs[TMC2660_DRVCONF] = TMC2660_DRVCONF_RDSEL_MSTEP;
  }

  // Setup pins
  // Must set the SS pin either in/high or any/output for master mode to work
  TMC2660_SPI_PORT.OUTSET = 1 << TMC2660_SPI_SS_PIN;   // High
  TMC2660_SPI_PORT.DIRCLR = 1 << TMC2660_SPI_SS_PIN;   // Input

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


uint8_t tmc2660_status(int driver) {
  return drivers[driver].status;
}


uint16_t tmc2660_step(int driver) {
  return drivers[driver].mstep;
}


void tmc2660_reset(int driver) {
  drivers[driver].reset = 1;
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


static void tmc2660_get_status_flags(uint8_t status, char buf[35]) {
  buf[0] = 0;

  if (TMC2660_DRVSTATUS_STST & status) strcat(buf, "stst,");
  if (TMC2660_DRVSTATUS_OLB  & status) strcat(buf, "olb,");
  if (TMC2660_DRVSTATUS_OLA  & status) strcat(buf, "ola,");
  if (TMC2660_DRVSTATUS_S2GB & status) strcat(buf, "s2gb,");
  if (TMC2660_DRVSTATUS_S2GA & status) strcat(buf, "s2ga,");
  if (TMC2660_DRVSTATUS_OTPW & status) strcat(buf, "otpw,");
  if (TMC2660_DRVSTATUS_OT   & status) strcat(buf, "ot,");
  if (TMC2660_DRVSTATUS_SG   & status) strcat(buf, "sg,");

  if (buf[0] != 0) buf[strlen(buf) - 1] = 0; // Remove last comma
}


static const char mst_fmt[] PROGMEM = "Motor %i step:       %i\n";
static const char mfl_fmt[] PROGMEM = "Motor %i flags:      %s\n";


void tmc2660_print_motor_step(nvObj_t *nv) {
  int driver = nv->token[3] - '1';
  fprintf_P(stderr, mst_fmt, driver + 1, drivers[driver].mstep);
}


void tmc2660_print_motor_flags(nvObj_t *nv) {
  int driver = nv->token[3] - '1';

  char buf[35];
  tmc2660_get_status_flags(drivers[driver].status, buf);

  fprintf_P(stderr, mfl_fmt, driver + 1, buf);
}


stat_t tmc2660_get_motor_step(nvObj_t *nv) {
  int driver = nv->token[3] - '1';

  nv->value = drivers[driver].mstep;
  nv->valuetype = TYPE_INTEGER;

  return STAT_OK;
}


stat_t tmc2660_get_motor_flags(nvObj_t *nv) {
  int driver = nv->token[3] - '1';
  char buf[35];

  tmc2660_get_status_flags(drivers[driver].status, buf);
  nv_copy_string(nv, buf);
  nv->valuetype = TYPE_STRING;

  return STAT_OK;
}
