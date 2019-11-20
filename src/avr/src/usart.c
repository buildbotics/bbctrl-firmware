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

#include "usart.h"
#include "cpp_magic.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>


// Ring buffers
#define RING_BUF_INDEX_TYPE volatile uint16_t
#define RING_BUF_NAME tx_buf
#define RING_BUF_SIZE USART_TX_RING_BUF_SIZE
#define RING_BUF_ATOMIC_COPY 1
#include "ringbuf.def"

#define RING_BUF_INDEX_TYPE volatile uint16_t
#define RING_BUF_NAME rx_buf
#define RING_BUF_SIZE USART_RX_RING_BUF_SIZE
#define RING_BUF_ATOMIC_COPY 1
#include "ringbuf.def"

static bool _flush = false;


static void _set_dre_interrupt(bool enable) {
  if (enable) SERIAL_PORT.CTRLA |= USART_DREINTLVL_MED_gc;
  else SERIAL_PORT.CTRLA &= ~USART_DREINTLVL_MED_gc;
}


static void _set_rxc_interrupt(bool enable) {
  if (enable) {
    if (SERIAL_CTS_THRESH <= rx_buf_space())
      OUTCLR_PIN(SERIAL_CTS_PIN); // CTS Lo (enable)

    SERIAL_PORT.CTRLA |= USART_RXCINTLVL_HI_gc;

  } else SERIAL_PORT.CTRLA &= ~USART_RXCINTLVL_HI_gc;
}


// Data register empty interrupt vector
ISR(SERIAL_DRE_vect) {
  if (tx_buf_empty()) _set_dre_interrupt(false); // Disable interrupt

  else {
    SERIAL_PORT.DATA = tx_buf_peek();
    tx_buf_pop();
  }
}


// Data received interrupt vector
ISR(SERIAL_RXC_vect) {
  if (rx_buf_full()) _set_rxc_interrupt(false); // Disable interrupt
  else rx_buf_push(SERIAL_PORT.DATA);

  if (rx_buf_space() < SERIAL_CTS_THRESH)
    OUTSET_PIN(SERIAL_CTS_PIN); // CTS Hi (disable)
}


#ifdef __AVR__
static int _usart_putchar(char c, FILE *f) {
  usart_putc(c);
  return 0;
}
#endif // __AVR__


static void _set_baud(USART_t *port, uint16_t bsel, uint8_t bscale) {
  port->BAUDCTRLB = (uint8_t)((bscale << 4) | (bsel >> 8));
  port->BAUDCTRLA = bsel;
  port->CTRLB |= USART_CLK2X_bm;
}


void usart_set_baud(USART_t *port, baud_t baud) {
  // The BSEL / BSCALE values provided below assume a 32 Mhz clock
  // With CTRLB CLK2X is set
  // See http://www.avrcalc.elektronik-projekt.de/xmega/baud_rate_calculator

  switch (baud) {
  case USART_BAUD_9600:    _set_baud(port, 3325, 0b1101); break;
  case USART_BAUD_19200:   _set_baud(port, 3317, 0b1100); break;
  case USART_BAUD_38400:   _set_baud(port, 3301, 0b1011); break;
  case USART_BAUD_57600:   _set_baud(port, 1095, 0b1100); break;
  case USART_BAUD_115200:  _set_baud(port, 1079, 0b1011); break;
  case USART_BAUD_230400:  _set_baud(port, 1047, 0b1010); break;
  case USART_BAUD_460800:  _set_baud(port, 983,  0b1001); break;
  case USART_BAUD_921600:  _set_baud(port, 107,  0b1011); break;
  case USART_BAUD_500000:  _set_baud(port, 1,    0b0010); break;
  case USART_BAUD_1000000: _set_baud(port, 1,    0b0001); break;
  }
}


void usart_set_parity(USART_t *port, parity_t parity) {
  uint8_t reg = port->CTRLC & ~USART_PMODE_gm;

  switch (parity) {
  case USART_NONE: reg |= USART_PMODE_DISABLED_gc; break;
  case USART_EVEN: reg |= USART_PMODE_EVEN_gc; break;
  case USART_ODD:  reg |= USART_PMODE_ODD_gc; break;
  }

  port->CTRLC = reg;
}


void usart_set_stop(USART_t *port, stop_t stop) {
  switch (stop) {
  case USART_1STOP: port->CTRLC &= ~USART_SBMODE_bm; break;
  case USART_2STOP: port->CTRLC |= USART_SBMODE_bm;  break;
  }
}


void usart_set_bits(USART_t *port, bits_t bits) {
  uint8_t reg = port->CTRLC & ~USART_CHSIZE_gm;

  switch (bits) {
  case USART_5BITS: reg |= USART_CHSIZE_5BIT_gc; break;
  case USART_6BITS: reg |= USART_CHSIZE_6BIT_gc; break;
  case USART_7BITS: reg |= USART_CHSIZE_7BIT_gc; break;
  case USART_8BITS: reg |= USART_CHSIZE_8BIT_gc; break;
  case USART_9BITS: reg |= USART_CHSIZE_9BIT_gc; break;
  }

  port->CTRLC = reg;
}


void usart_init_port(USART_t *port, baud_t baud, parity_t parity, bits_t bits,
                     stop_t stop) {
  // Set baud rate
  usart_set_baud(port, baud);

  // Async, no parity, 8 data bits, 1 stop bit
  port->CTRLC = USART_CMODE_ASYNCHRONOUS_gc;
  usart_set_parity(port, parity);
  usart_set_bits(port, bits);
  usart_set_stop(port, stop);

  // Configure receiver and transmitter
  port->CTRLB |= USART_RXEN_bm | USART_TXEN_bm;
}


void usart_init() {
  // Setup ring buffer
  tx_buf_init();
  rx_buf_init();

  PR.PRPC &= ~PR_USART0_bm; // Disable power reduction

  // Setup pins
  OUTSET_PIN(SERIAL_CTS_PIN); // CTS Hi (disable)
  DIRSET_PIN(SERIAL_CTS_PIN); // CTS Output
  OUTSET_PIN(SERIAL_TX_PIN);  // Tx High
  DIRSET_PIN(SERIAL_TX_PIN);  // Tx Output
  DIRCLR_PIN(SERIAL_RX_PIN);  // Rx Input

  // Configure port
  usart_init_port(&SERIAL_PORT, SERIAL_BAUD, USART_NONE, USART_8BITS,
                  USART_1STOP);

  PMIC.CTRL |= PMIC_HILVLEN_bm; // Interrupt level on

#ifdef __AVR__
  // Connect IO
  static FILE _stdout;
  memset(&_stdout, 0, sizeof(FILE));
  _stdout.put = _usart_putchar;
  _stdout.flags = _FDEV_SETUP_WRITE;

  stdout = &_stdout;
  stderr = &_stdout;
#endif // __AVR__

  // Enable Rx
  _set_rxc_interrupt(true);
}



void usart_putc(char c) {
  while (tx_buf_full() || _flush) continue;
  tx_buf_push(c);
  _set_dre_interrupt(true); // Enable interrupt
}


void usart_puts(const char *s) {while (*s) usart_putc(*s++);}


int8_t usart_getc() {
  while (rx_buf_empty()) continue;
  uint8_t data = rx_buf_next();
  _set_rxc_interrupt(true); // Enable interrupt
  return data;
}


/*** Line editing features:
 *
 *   ENTER     Submit current command line.
 *   BS        Backspace, delete last character.
 *   CTRL-X    Cancel current line entry.
 */
char *usart_readline() {
  static char line[INPUT_BUFFER_LEN];
  static int i = 0;
  bool eol = false;

  while (!rx_buf_empty()) {
    char data = usart_getc();

    switch (data) {
    case '\r': case '\n': eol = true; break;
    case '\b': if (i) i--; break; // BS - backspace
    case 0x18: i = 0; break;      // CAN - Cancel or CTRL-X

    default:
      line[i++] = data;
      if (i == INPUT_BUFFER_LEN - 1) eol = true; // Line buffer full
      break;
    }

    if (eol) {
      line[i] = 0;
      i = 0;
      return line;
    }
  }

  return 0;
}


void usart_flush() {
  _flush = true;

  while (!tx_buf_empty() || !(SERIAL_PORT.STATUS & USART_DREIF_bm) ||
         !(SERIAL_PORT.STATUS & USART_TXCIF_bm))
    continue;
}


void usart_rx_flush() {rx_buf_init();}
int16_t usart_rx_space() {return rx_buf_space();}
int16_t usart_rx_fill() {return rx_buf_fill();}
int16_t usart_tx_space() {return tx_buf_space();}
int16_t usart_tx_fill() {return tx_buf_fill();}
