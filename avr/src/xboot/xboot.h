/************************************************************************/
/* XBoot Extensible AVR Bootloader                                      */
/*                                                                      */
/* xboot.h                                                              */
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

#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/crc16.h>

// token pasting
#define CAT2_int(x, y) x ## y
#define CAT2(x, y) CAT2_int(x, y)
#define CAT3_int(x, y, z) x ## y ## z
#define CAT3(x, y, z) CAT3_int(x, y, z)

// Version
#define XBOOT_VERSION_MAJOR 1
#define XBOOT_VERSION_MINOR 7

// Configuration
#define ENTER_WAIT 1000 // In ms
#define UART_BAUD_RATE 115200
#define UART_NUMBER 0
#define UART_PORT_NAME C
#define USE_AVR1008_EEPROM
#define USE_DFLL
#define WATCHDOG_TIMEOUT WDT_PER_1KCLK_gc

// clock config
// use 32MHz osc if makefile calls for it
#if (F_CPU == 32000000L)
// defaults to 2MHz RC oscillator
// define USE_32MHZ_RC to override
#define USE_32MHZ_RC
#endif // F_CPU

// UART RS485 Enable Output
#define UART_EN_PORT            CAT2(PORT, UART_EN_PORT_NAME)

#if (UART_NUMBER == 0)
#define UART_RX_PIN             2
#define UART_TX_PIN             3
#else
#define UART_RX_PIN             6
#define UART_TX_PIN             7
#endif
#define UART_PORT               CAT2(PORT, UART_PORT_NAME)
#define UART_DEVICE_PORT        CAT2(UART_PORT_NAME, UART_NUMBER)
#define UART_DEVICE             CAT2(USART, UART_DEVICE_PORT)
#define UART_DEVICE_RXC_ISR     CAT3(USART, UART_DEVICE_PORT, _RXC_vect)
#define UART_DEVICE_DRE_ISR     CAT3(USART, UART_DEVICE_PORT, _DRE_vect)
#define UART_DEVICE_TXC_ISR     CAT3(USART, UART_DEVICE_PORT, _TXC_vect)
#define UART_RX_PIN_CTRL        CAT3(UART_PORT.PIN, UART_RX_PIN, CTRL)
#define UART_TX_PIN_CTRL        CAT3(UART_PORT.PIN, UART_TX_PIN, CTRL)

// BAUD Rate Values
// Known good at 2MHz
#if (F_CPU == 2000000L) && (UART_BAUD_RATE == 19200)
#define UART_BSEL_VALUE         12
#define UART_BSCALE_VALUE       0
#define UART_CLK2X              1
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 38400)
#define UART_BSEL_VALUE         22
#define UART_BSCALE_VALUE       -2
#define UART_CLK2X              1
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 57600)
#define UART_BSEL_VALUE         26
#define UART_BSCALE_VALUE       -3
#define UART_CLK2X              1
#elif (F_CPU == 2000000L) && (UART_BAUD_RATE == 115200)
#define UART_BSEL_VALUE         19
#define UART_BSCALE_VALUE       -4
#define UART_CLK2X              1

// Known good at 32MHz
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 19200)
#define UART_BSEL_VALUE         103
#define UART_BSCALE_VALUE       0
#define UART_CLK2X              0
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 38400)
#define UART_BSEL_VALUE         51
#define UART_BSCALE_VALUE       0
#define UART_CLK2X              0
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 57600)
#define UART_BSEL_VALUE         34
#define UART_BSCALE_VALUE       0
#define UART_CLK2X              0
#elif (F_CPU == 32000000L) && (UART_BAUD_RATE == 115200)
#define UART_BSEL_VALUE         1047
#define UART_BSCALE_VALUE       -6
#define UART_CLK2X              0

#else // None of the above, so calculate something
#warning Not using predefined BAUD rate, possible BAUD rate error!
#if (F_CPU == 2000000L)
#define UART_BSEL_VALUE         ((F_CPU) / ((uint32_t)UART_BAUD_RATE * 8) - 1)
#define UART_BSCALE_VALUE       0
#define UART_CLK2X              1

#else
#define UART_BSEL_VALUE         ((F_CPU) / ((uint32_t)UART_BAUD_RATE * 16) - 1)
#define UART_BSCALE_VALUE       0
#define UART_CLK2X              0
#endif
#endif

#ifndef EEPROM_PAGE_SIZE
#define EEPROM_PAGE_SIZE E2PAGESIZE
#endif

#ifndef EEPROM_BYTE_ADDRESS_MASK
#if EEPROM_PAGE_SIZE == 32
#define EEPROM_BYTE_ADDRESS_MASK 0x1f
#elif EEPROM_PAGE_SIZE == 16
#define EEPROM_BYTE_ADDRESS_MASK = 0x0f
#elif EEPROM_PAGE_SIZE == 8
#define EEPROM_BYTE_ADDRESS_MASK = 0x07
#elif EEPROM_PAGE_SIZE == 4
#define EEPROM_BYTE_ADDRESS_MASK = 0x03
#else
#error Unknown EEPROM page size!  Please add new byte address value!
#endif
#endif

// Includes
#include "protocol.h"
#include "sp_driver.h"
#include "eeprom_driver.h"
#include "uart.h"
#include "watchdog.h"

// Functions
uint8_t get_char(void);
void send_char(uint8_t c);
uint16_t get_word(void);

uint8_t BlockLoad(unsigned size, uint8_t mem, uint32_t *address);
void BlockRead(unsigned size, uint8_t mem, uint32_t *address);

uint16_t crc16_block(uint32_t start, uint32_t length);
