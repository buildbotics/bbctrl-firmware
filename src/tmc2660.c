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
  uint16_t mstep;
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


static volatile tmc2660_state_t state;
static volatile uint8_t driver;
static volatile uint8_t reg;
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
  // Flush any status errors
  // TODO check errors
  uint8_t x = SPIC.STATUS;
  x = x;

  // Read
  if (!spi_byte) spi_in = 0;
  else spi_in = spi_in << 8 | SPIC.DATA;

  // Write
  if (spi_byte < 3)
    SPIC.DATA = 0xff & (spi_out >> ((2 - spi_byte++) * 8));
  else spi_byte = 0;
}


void spi_next() {
  switch (spi_state) {
  case SPI_STATE_SELECT:
    // Select driver
    spi_cs(driver, 1);

    // Next state
    TMC2660_TIMER.PER = 4;
    spi_state = SPI_STATE_WRITE;
    break;

  case SPI_STATE_WRITE:
    spi_out = reg_addrs[reg] | drivers[driver].regs[reg];
    spi_send();

    // Next state
    TMC2660_TIMER.PER = 16;
    spi_state = SPI_STATE_READ;
    break;

  case SPI_STATE_READ:
    // Deselect driver
    spi_cs(driver, 0);

    // Read response
    drivers[driver].mstep = (spi_in >> 14) & 0x3ff;
    drivers[driver].status = spi_in >> 4;

    // Next state
    spi_state = SPI_STATE_SELECT;

    if (++reg == 5) {
      reg = 0;
      if (++driver == TMC2660_NUM_DRIVERS) driver = 0;

      TMC2660_TIMER.PER = 10;

    } else TMC2660_TIMER.PER = 2;
    break;
  }
}


ISR(SPIC_INT_vect) {
  spi_send();
}


ISR(TCC1_OVF_vect) {
  spi_next();
}


void tmc2660_init() {
  // Reset state
  state = TMC2660_STATE_CONFIG;
  spi_state = SPI_STATE_SELECT;
  driver = reg = spi_byte = 0;
  memset(drivers, 0, sizeof(drivers));

  // Configure motors
  for (int i = 0; i < TMC2660_NUM_DRIVERS; i++) {
    drivers[i].regs[TMC2660_DRVCTRL] = 0;
    drivers[i].regs[TMC2660_CHOPCONF] = 0x14557;
    drivers[i].regs[TMC2660_SMARTEN] = 0x8202;
    drivers[i].regs[TMC2660_SGCSCONF] = 0x1001f;
    drivers[i].regs[TMC2660_DRVCONF] = 0x10;
  }

  // Setup pins
  TMC2660_SPI_PORT.OUTSET = 1 << 4;  // High
  TMC2660_SPI_PORT.DIRSET = 1 << 4;  // Output
  TMC2660_SPI_PORT.OUTSET = 1 << TMC2660_SPI_SCK_PIN;  // High
  TMC2660_SPI_PORT.DIRSET = 1 << TMC2660_SPI_SCK_PIN;  // Output
  TMC2660_SPI_PORT.DIRCLR = 1 << TMC2660_SPI_MISO_PIN; // Input
  TMC2660_SPI_PORT.OUTSET = 1 << TMC2660_SPI_MOSI_PIN; // High
  TMC2660_SPI_PORT.DIRSET = 1 << TMC2660_SPI_MOSI_PIN; // Output

#if TMC2660_NUM_DRIVERS > 0
  TMC2660_SPI_SSX_PORT.OUTSET = 1 << TMC2660_SPI_SSX_PIN; // High
  TMC2660_SPI_SSX_PORT.DIRSET = 1 << TMC2660_SPI_SSX_PIN; // Output
#endif
#if TMC2660_NUM_DRIVERS > 1
  TMC2660_SPI_SSY_PORT.OUTSET = 1 << TMC2660_SPI_SSY_PIN; // High
  TMC2660_SPI_SSY_PORT.DIRSET = 1 << TMC2660_SPI_SSY_PIN; // Output
#endif
#if TMC2660_NUM_DRIVERS > 2
  TMC2660_SPI_SSZ_PORT.OUTSET = 1 << TMC2660_SPI_SSZ_PIN; // High
  TMC2660_SPI_SSZ_PORT.DIRSET = 1 << TMC2660_SPI_SSZ_PIN; // Output
#endif
#if TMC2660_NUM_DRIVERS > 3
  TMC2660_SPI_SSA_PORT.OUTSET = 1 << TMC2660_SPI_SSA_PIN; // High
  TMC2660_SPI_SSA_PORT.DIRSET = 1 << TMC2660_SPI_SSA_PIN; // Output
#endif
#if TMC2660_NUM_DRIVERS > 4
  TMC2660_SPI_SSB_PORT.OUTSET = 1 << TMC2660_SPI_SSB_PIN; // High
  TMC2660_SPI_SSB_PORT.DIRSET = 1 << TMC2660_SPI_SSB_PIN; // Output
#endif

  // Configure SPI
  PR.PRPC &= ~PR_SPI_bm; // Disable power reduction
  SPIC.CTRL = SPI_ENABLE_bm | SPI_DORD_bm | SPI_MASTER_bm | SPI_MODE_3_gc |
    SPI_PRESCALER_DIV128_gc; // enable, big endian, master, mode 3, clock/128
  PORTC.REMAP = PORT_SPI_bm; // Swap SCK and MOSI
  SPIC.INTCTRL = SPI_INTLVL_MED_gc; // interupt level

  // Configure timer
  PR.PRPC &= ~PR_TC1_bm; // Disable power reduction
  TMC2660_TIMER.PER = F_CPU / 1024 / 10; // Set timer period
  TMC2660_TIMER.INTCTRLA = TC_OVFINTLVL_LO_gc; // Low priority overflow int
  TMC2660_TIMER.CTRLA = TC_CLKSEL_DIV1024_gc; // enable, clock/1024
}


uint8_t tmc2660_status(int driver) {
  return drivers[driver].status;
}


uint16_t tmc2660_step(int driver) {
  return drivers[driver].mstep;
}
