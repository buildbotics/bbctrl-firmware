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

/*******************************************************************************
 * $Revision: 1691 $
 * $Date: 2008-07-29 13:25:40 +0200 (ti, 29 jul 2008) $  \n
 *
 * Copyright (c) 2008, Atmel Corporation All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of ATMEL may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY AND
 * SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*! \file **********************************************************************
 * \brief XMEGA Self-programming driver header file.
 *
 *      This file contains the function prototypes for the
 *      XMEGA Self-programming driver.
 *      If any SPM instructions are used, the linker file must define
 *      a segment named BOOT which must be located in the device boot section.
 *
 *
 *      None of these functions clean up the NVM Command Register after use.
 *      It is therefore important to write NVMCMD_NO_OPERATION (0x00) to this
 *      register when you are finished using any of the functions in this
 *      driver.
 *
 *      For all functions, it is important that no interrupt handlers perform
 *      any NVM operations. The user must implement a scheme for mutually
 *      exclusive access to the NVM. However, the 4-cycle timeout will work
 *      fine, since writing to the Configuration Change Protection register
 *      (CCP) automatically disables interrupts for 4 instruction cycles.
 *
 * \par Application note: AVR1316: XMEGA Self-programming
 *
 * \par Documentation
 *      For comprehensive code documentation, supported compilers, compiler
 *      settings and supported devices see readme.html
 *
 * \author
 *      Atmel Corporation: http://www.atmel.com
 *      Support email: avr@atmel.com
 ******************************************************************************/

#pragma once

#include <avr/io.h>

#include <stdint.h>

#ifndef APP_SECTION_PAGE_SIZE
#error APP_SECTION_PAGE_SIZE must be defined if not defined in header files.
#endif

#ifndef APPTABLE_SECTION_START
#error APPTABLE_SECTION_START must be defined if not defined in header files.
#endif


/*! \brief Read a byte from flash.
 *
 *  \param address Address to the location of the byte to read.
 *  \retval Byte read from flash.
 */
uint8_t SP_ReadByte(uint32_t address);

/*! \brief Read a word from flash.
 *
 *  This function reads one word from the flash.
 *
 *  \param address Address to the location of the word to read.
 *
 *  \retval word read from flash.
 */
uint16_t SP_ReadWord(uint32_t address);

/*! \brief Read calibration byte at given index.
 *
 *  This function reads one calibration byte from the Calibration signature row.
 *
 *  \param index  Index of the byte in the calibration signature row.
 *
 *  \retval Calibration byte
 */
uint8_t SP_ReadCalibrationByte(uint8_t index);

/*! \brief Read fuse byte from given index.
 *
 *  This function reads the fuse byte at the given index.
 *
 *  \param index  Index of the fuse byte.
 *  \retval Fuse byte
 */
uint8_t SP_ReadFuseByte(uint8_t index);

/*! \brief Read user signature at given index.
 *
 *  \param index  Index of the byte in the user signature row.
 *  \retval User signature byte
 */
uint8_t SP_ReadUserSignatureByte(uint16_t index);

/// Erase user signature row.
void SP_EraseUserSignatureRow();

/// Write user signature row.
void SP_WriteUserSignatureRow();

/*! \brief Erase entire application section.
 *
 *  \note If the lock bits is set to not allow SPM in the application or
 *        application table section the erase is not done.
 */
void SP_EraseApplicationSection();

/*! \brief Erase and write page buffer to application or application table
 *  section at byte address.
 *
 *  \param address Byte address for flash page.
 */
void SP_EraseWriteApplicationPage(uint32_t address);

/*! \brief Write page buffer to application or application table section at
 *  byte address.
 *
 *  \note The page that is written to must be erased before it is written to.
 *
 *  \param address Byte address for flash page.
 */
void SP_WriteApplicationPage(uint32_t address);

/*! \brief Load one word into Flash page buffer.
 *
 *  \param  address   Position in inside the flash page buffer.
 *  \param  data      Value to be put into the buffer.
 */
void SP_LoadFlashWord(uint16_t address, uint16_t data);

/*! \brief Load entire page from SRAM buffer into Flash page buffer.
 *
 *	\param data   Pointer to the data to put in buffer.
 *
 *	\note The __near keyword limits the pointer to two bytes which means that
 *        only data up to 64K (internal SRAM) can be used.
 */
void SP_LoadFlashPage(const uint8_t *data);

/// Flush Flash page buffer.
void SP_EraseFlashBuffer();

/// Disables the use of SPM until the next reset.
void SP_LockSPM();

/// Waits for the SPM to finish and clears the command register.
void SP_WaitForSPM();
