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

#ifndef TMC2660_H
#define TMC2660_H

#include <stdint.h>

#define TMC2660_SPI_PORT PORTC
#define TMC2660_SPI_SCK_PIN 5
#define TMC2660_SPI_MISO_PIN 6
#define TMC2660_SPI_MOSI_PIN 7

#define TMC2660_SPI_SSX_PORT PORTA
#define TMC2660_SPI_SSX_PIN 3
#define TMC2660_SPI_SSY_PORT PORTF
#define TMC2660_SPI_SSY_PIN 3
#define TMC2660_SPI_SSZ_PORT PORTE
#define TMC2660_SPI_SSZ_PIN 3
#define TMC2660_SPI_SSA_PORT PORTD
#define TMC2660_SPI_SSA_PIN 3
#define TMC2660_SPI_SSB_PORT PORTB
#define TMC2660_SPI_SSB_PIN 3

#define TMC2660_NUM_DRIVERS 4

#define TMC2660_TIMER TCC1

void tmc2660_init();
uint8_t tmc2660_status(int driver);
uint16_t tmc2660_step(int driver);

#define TMC2660_DRVCTRL             0
#define TMC2660_DRVCTRL_ADDR        (0UL << 18)
#define TMC2660_DRVCTRL_PHA         (1UL << 17)
#define TMC2660_DRVCTRL_CA(x)       (((int32_t)x & 0xff) << 9)
#define TMC2660_DRVCTRL_PHB         (1UL << 8)
#define TMC2660_DRVCTRL_CB(x)       (((int32_t)x & 0xff) << 0)
#define TMC2660_DRVCTRL_INTPOL      (1UL << 9)
#define TMC2660_DRVCTRL_DEDGE       (1UL << 8)
#define TMC2660_DRVCTRL_MRES_256    (0UL << 0)
#define TMC2660_DRVCTRL_MRES_128    (1UL << 0)
#define TMC2660_DRVCTRL_MRES_64     (2UL << 0)
#define TMC2660_DRVCTRL_MRES_32     (3UL << 0)
#define TMC2660_DRVCTRL_MRES_16     (4UL << 0)
#define TMC2660_DRVCTRL_MRES_8      (5UL << 0)
#define TMC2660_DRVCTRL_MRES_4      (6UL << 0)
#define TMC2660_DRVCTRL_MRES_2      (7UL << 0)
#define TMC2660_DRVCTRL_MRES_1      (8UL << 0)

#define TMC2660_CHOPCONF            1
#define TMC2660_CHOPCONF_ADDR       (4UL << 17)
#define TMC2660_CHOPCONF_TBL_16     (0UL << 15)
#define TMC2660_CHOPCONF_TBL_24     (1UL << 15)
#define TMC2660_CHOPCONF_TBL_36     (2UL << 15)
#define TMC2660_CHOPCONF_TBL_54     (3UL << 15)
#define TMC2660_CHOPCONF_CHM        (1UL << 14)
#define TMC2660_CHOPCONF_RNDTF      (1UL << 13)
#define TMC2660_CHOPCONF_FDM_COMP   (0UL << 12)
#define TMC2660_CHOPCONF_FDM_TIMER  (1UL << 12)
#define TMC2660_CHOPCONF_HDEC_16    (0UL << 11)
#define TMC2660_CHOPCONF_HDEC_32    (1UL << 11)
#define TMC2660_CHOPCONF_HDEC_48    (2UL << 11)
#define TMC2660_CHOPCONF_HDEC_64    (3UL << 11)
#define TMC2660_CHOPCONF_HEND(x)    ((((int32_t)x + 3) & 0xf) << 7)
#define TMC2660_CHOPCONF_SWO(x)     ((((int32_t)x + 3) & 0xf) << 7)
#define TMC2660_CHOPCONF_HSTART(x)  ((((int32_t)x - 1) & 7) << 4)
#define TMC2660_CHOPCONF_FASTD(x)   ((((int32_t)x & 8) << 11) | (x & 7) << 4))
#define TMC2660_CHOPCONF_TOFF_TBL   (1 << 0)
#define TMC2660_CHOPCONF_TOFF(x)    (((int32_t)x & 0xf) << 0)

#define TMC2660_SMARTEN             2
#define TMC2660_SMARTEN_ADDR        (5UL << 17)
#define TMC2660_SMARTEN_SEIMIN      (1UL << 15)
#define TMC2660_SMARTEN_SEDN_32     (0UL << 13)
#define TMC2660_SMARTEN_SEDN_8      (1UL << 13)
#define TMC2660_SMARTEN_SEDN_2      (2UL << 13)
#define TMC2660_SMARTEN_SEDN_1      (3UL << 13)
#define TMC2660_SMARTEN_MAX(x)      ((x & 0xf) << 8)
#define TMC2660_SMARTEN_SEUP_1      (0UL << 5)
#define TMC2660_SMARTEN_SEUP_2      (1UL << 5)
#define TMC2660_SMARTEN_SEUP_4      (2UL << 5)
#define TMC2660_SMARTEN_SEUP_8      (3UL << 5)
#define TMC2660_SMARTEN_MIN(x)      (((int32_t)x & 0xf) << 0)

#define TMC2660_SGCSCONF            3
#define TMC2660_SGCSCONF_ADDR       (6UL << 17)
#define TMC2660_SGCSCONF_SFILT      (1UL << 17)
#define TMC2660_SGCSCONF_THRESH(x)  (((int32_t)x & 0x7f) << 8)
#define TMC2660_SGCSCONF_CS(x)      (((int32_t)x & 0x1f) << 0)
#define TMC2660_SGCSCONF_CS_NONE    (31UL << 0)

#define TMC2660_DRVCONF             4
#define TMC2660_DRVCONF_ADDR        (7UL << 17)
#define TMC2660_DRVCONF_TST         (1UL << 16)
#define TMC2660_DRVCONF_SLPH_MIN    (0UL << 14)
#define TMC2660_DRVCONF_SLPH_MIN_TC (1UL << 14)
#define TMC2660_DRVCONF_SLPH_MED_TC (2UL << 14)
#define TMC2660_DRVCONF_SLPH_MAX    (3UL << 14)
#define TMC2660_DRVCONF_SLPL_MIN    (0UL << 12)
#define TMC2660_DRVCONF_SLPL_MED    (2UL << 12)
#define TMC2660_DRVCONF_SLPL_MAX    (3UL << 12)
#define TMC2660_DRVCONF_DISS2G      (1UL << 10)
#define TMC2660_DRVCONF_TS2G_3_2    (0UL << 8)
#define TMC2660_DRVCONF_TS2G_1_6    (1UL << 8)
#define TMC2660_DRVCONF_TS2G_1_2    (2UL << 8)
#define TMC2660_DRVCONF_TS2G_0_8    (3UL << 8)
#define TMC2660_DRVCONF_SDOFF       (1UL << 7)
#define TMC2660_DRVCONF_VSENSE      (1UL << 6)
#define TMC2660_DRVCONF_RDSEL_MSTEP (0UL << 4)
#define TMC2660_DRVCONF_RDSEL_SG    (1UL << 4)
#define TMC2660_DRVCONF_RDSEL_SGCS  (2UL << 4)

#define TMC2660_DRVSTATUS_STST      (1UL << 7)
#define TMC2660_DRVSTATUS_OLB       (1UL << 6)
#define TMC2660_DRVSTATUS_OLA       (1UL << 5)
#define TMC2660_DRVSTATUS_S2GB      (1UL << 4)
#define TMC2660_DRVSTATUS_S2GA      (1UL << 3)
#define TMC2660_DRVSTATUS_OTPW      (1UL << 2)
#define TMC2660_DRVSTATUS_OT        (1UL << 1)
#define TMC2660_DRVSTATUS_SG        (1UL << 0)

#endif // TMC2660_H
