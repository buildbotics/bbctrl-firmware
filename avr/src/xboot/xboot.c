/************************************************************************/
/* XBoot Extensible AVR Bootloader                                      */
/*                                                                      */
/* xboot.c                                                              */
/*                                                                      */
/* Alex Forencich <alex@alexforencich.com>                              */
/*                                                                      */
/* Copyright (c) 2010 Alex Forencich                                    */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation       */
/* files(the "Software"), to deal in the Software without restriction,  */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies or substantial portions of the Software.      */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#include "xboot.h"

#include <util/delay.h>

#include <stdbool.h>


uint8_t buffer[SPM_PAGESIZE];


void NVM_Wait() {
  while (NVM_STATUS & NVM_NVMBUSY_bp)
    // reset watchdog while waiting for erase completion
    WDT_Reset();
}


int main() {
  // Initialization section
  // Entry point and communication methods are initialized here
  // --------------------------------------------------
#ifdef USE_32MHZ_RC
#if (F_CPU != 32000000L)
#error F_CPU must match oscillator setting!
#endif // F_CPU
  OSC.CTRL |= OSC_RC32MEN_bm; // turn on 32 MHz oscillator
  while (!(OSC.STATUS & OSC_RC32MRDY_bm)) continue; // wait for it to start
  CCP = CCP_IOREG_gc;
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc;
#ifdef USE_DFLL
  OSC.CTRL |= OSC_RC32KEN_bm;
  while(!(OSC.STATUS & OSC_RC32KRDY_bm));
  DFLLRC32M.CTRL = DFLL_ENABLE_bm;
#endif // USE_DFLL
#else // USE_32MHZ_RC
#if (F_CPU != 2000000L)
#error F_CPU must match oscillator setting!
#endif // F_CPU
#ifdef USE_DFLL
  OSC.CTRL |= OSC_RC32KEN_bm;
  while(!(OSC.STATUS & OSC_RC32KRDY_bm));
  DFLLRC2M.CTRL = DFLL_ENABLE_bm;
#endif // USE_DFLL
#endif // USE_32MHZ_RC

  // Initialize UART
  uart_init();

  // End initialization section

  uint32_t address = 0;
  uint16_t i = 0;

  bool in_bootloader = false;
  uint16_t j = ENTER_WAIT;
  while (!in_bootloader && 0 < j--) {
    // Check for trigger
    if (uart_has_char()) in_bootloader = uart_recv_char() == CMD_SYNC;

    _delay_ms(1);
  }

  WDT_EnableAndSetTimeout();

  // Main bootloader
  while (in_bootloader) {
    uint8_t val = get_char();
    WDT_Reset();

    // Main bootloader parser
    switch (val) {
    case CMD_CHECK_AUTOINCREMENT: send_char(REPLY_YES); break;

    case CMD_SET_ADDRESS:
      address = get_word();
      send_char(REPLY_ACK);
      break;

    case CMD_SET_EXT_ADDRESS:
      address = ((uint32_t)get_char() << 16) | get_word();
      send_char(REPLY_ACK);
      break;

    case CMD_CHIP_ERASE:
      // Erase the application section
      SP_EraseApplicationSection();

      // Wait for completion
      NVM_Wait();

      // Erase EEPROM
      EEPROM_erase_all();

      send_char(REPLY_ACK);
      break;

    case CMD_CHECK_BLOCK_SUPPORT:
      send_char(REPLY_YES);
      // Send block size (page size)
      send_char((SPM_PAGESIZE >> 8) & 0xFF);
      send_char(SPM_PAGESIZE & 0xFF);
      break;

    case CMD_BLOCK_LOAD:
      i = get_word(); // Block size
      val = get_char(); // Memory type
      send_char(BlockLoad(i, val, &address)); // Load it
      break;

    case CMD_BLOCK_READ:
      i = get_word(); // Block size
      val = get_char(); // Memory type
      BlockRead(i, val, &address); // Read it
      break;

    case CMD_READ_BYTE: {
      unsigned w = SP_ReadWord((address << 1));

      send_char(w >> 8);
      send_char(w);

      address++;
      break;
    }

    case CMD_WRITE_LOW_BYTE:
      i = get_char(); // get low byte
      send_char(REPLY_ACK);
      break;

    case CMD_WRITE_HIGH_BYTE:
      i |= (get_char() << 8); // get high byte; combine
      SP_LoadFlashWord((address << 1), i);
      address++;
      send_char(REPLY_ACK);
      break;

    case CMD_WRITE_PAGE:
      if (address >= (APP_SECTION_SIZE >> 1))
        send_char(REPLY_ERROR); // don't allow bootloader overwrite

      else {
        SP_WriteApplicationPage(address << 1);
        send_char(REPLY_ACK);
      }
      break;

    case CMD_WRITE_EEPROM_BYTE:
      EEPROM_write_byte(address, get_char());
      address++;
      send_char(REPLY_ACK);
      break;

    case CMD_READ_EEPROM_BYTE:
      send_char(EEPROM_read_byte(address));
      address++;
      break;

    case CMD_READ_LOW_FUSE_BITS: send_char(SP_ReadFuseByte(0)); break;
    case CMD_READ_HIGH_FUSE_BITS: send_char(SP_ReadFuseByte(1)); break;
    case CMD_READ_EXT_FUSE_BITS: send_char(SP_ReadFuseByte(2)); break;

    case CMD_ENTER_PROG_MODE: case CMD_LEAVE_PROG_MODE:
      send_char(REPLY_ACK);
      break;

    case CMD_EXIT_BOOTLOADER:
      in_bootloader = false;
      send_char(REPLY_ACK);
      break;

    case CMD_PROGRAMMER_TYPE: send_char('S'); break; // serial

    case CMD_DEVICE_CODE:
      send_char(123); // send only this device
      send_char(0); // terminator
      break;

    case CMD_SET_LED: case CMD_CLEAR_LED: case CMD_SET_TYPE:
      get_char(); // discard parameter
      send_char(REPLY_ACK);
      break;

    case CMD_PROGRAM_ID:
      send_char('X');
      send_char('B');
      send_char('o');
      send_char('o');
      send_char('t');
      send_char('+');
      send_char('+');
      break;

    case CMD_VERSION:
      send_char('0' + XBOOT_VERSION_MAJOR);
      send_char('0' + XBOOT_VERSION_MINOR);
      break;

    case CMD_READ_SIGNATURE:
      send_char(SIGNATURE_2);
      send_char(SIGNATURE_1);
      send_char(SIGNATURE_0);
      break;

    case CMD_CRC: {
      uint32_t start = 0;
      uint32_t length = 0;
      uint16_t crc;

      val = get_char();

      switch (val) {
      case SECTION_FLASH: length = PROGMEM_SIZE; break;
      case SECTION_APPLICATION: length = APP_SECTION_SIZE; break;
      case SECTION_BOOT:
        start = BOOT_SECTION_START;
        length = BOOT_SECTION_SIZE;
        break;

      default:
        send_char(REPLY_ERROR);
        continue;
      }

      crc = crc16_block(start, length);

      send_char((crc >> 8) & 0xff);
      send_char(crc & 0xff);
      break;
    }

    case CMD_SYNC: break; // ESC (0x1b) to sync

    default: // otherwise, error
      send_char(REPLY_ERROR);
      break;
    }

    // Wait for any lingering SPM instructions to finish
    NVM_Wait();
  }

  // Bootloader exit
  // Code here runs after the bootloader has exited,
  // but before the application code has started

  // Shut down UART
  uart_deinit();

  WDT_Disable();

  // Jump into main code
  asm("jmp 0");
}


uint8_t get_char() {
  while (!uart_has_char()) continue;
  return uart_recv_char();
}


void send_char(uint8_t c) {uart_send_char_blocking(c);}
uint16_t get_word() {return (get_char() << 8) | get_char();}


uint8_t BlockLoad(unsigned size, uint8_t mem, uint32_t *address) {
  WDT_Reset();

  // fill up buffer
  for (int i = 0; i < SPM_PAGESIZE; i++) {
    char c = 0xff;
    if (i < size) c = get_char();
    buffer[i] = c;
  }

  switch (mem) {
  case MEM_EEPROM:
    EEPROM_write_block(*address, buffer, size);
    *address += size;
    break;

  case MEM_FLASH:
    // NOTE: Flash programming, address is given in words.
    SP_LoadFlashPage(buffer);
    SP_EraseWriteApplicationPage(*address << 1);
    *address += size >> 1;
    NVM_Wait();
    break;

  case MEM_USERSIG:
    SP_LoadFlashPage(buffer);
    SP_EraseUserSignatureRow();
    NVM_Wait();
    SP_WriteUserSignatureRow();
    NVM_Wait();
    break;

  default: return REPLY_ERROR;
  }

  return REPLY_ACK;
}



void BlockRead(unsigned size, uint8_t mem, uint32_t *address) {
  int offset = 0;
  int size2 = size;

  switch (mem) {
  case MEM_EEPROM:
    EEPROM_read_block(*address, buffer, size);
    (*address) += size;
    break;

  case MEM_FLASH: case MEM_USERSIG: case MEM_PRODSIG: {
    *address <<= 1; // Convert address to bytes temporarily

    do {
      switch (mem) {
      case MEM_FLASH: buffer[offset++] = SP_ReadByte(*address); break;

      case MEM_USERSIG:
        buffer[offset++] = SP_ReadUserSignatureByte(*address);
        break;

      case MEM_PRODSIG:
        buffer[offset++] = SP_ReadCalibrationByte(*address);
        break;
      }

      NVM_Wait();

      (*address)++;    // Select next word in memory.
      size--;          // Subtract two bytes from number of bytes to read
    } while (size);    // Repeat until all block has been read

    *address >>= 1;    // Convert address back to Flash words again.
    break;
  }

  default: send_char(REPLY_ERROR); break;
  }

  // send bytes
  for (int i = 0; i < size2; i++)
    send_char(buffer[i]);
}


uint16_t crc16_block(uint32_t start, uint32_t length) {
  uint16_t crc = 0;

  int bc = SPM_PAGESIZE;

  for (; length > 0; length--) {
    if (bc == SPM_PAGESIZE) {
      SP_ReadFlashPage(buffer, start);
      start += SPM_PAGESIZE;
      bc = 0;
    }

    crc = _crc16_update(crc, buffer[bc]);

    bc++;
  }

  return crc;
}
