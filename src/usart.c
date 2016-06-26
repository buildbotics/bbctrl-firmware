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

#include "usart.h"
#include "config.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdio.h>
#include <stdbool.h>

// Ring buffers
#define RING_BUF_NAME tx_buf
#define RING_BUF_SIZE USART_TX_RING_BUF_SIZE
#include "ringbuf.def"

#define RING_BUF_NAME rx_buf
#define RING_BUF_SIZE USART_RX_RING_BUF_SIZE
#include "ringbuf.def"

#define RING_BUF_NAME echo_buf
#define RING_BUF_SIZE USART_ECHO_RING_BUF_SIZE
#include "ringbuf.def"

static int usart_flags = USART_CRLF | USART_ECHO;


static void _set_dre_interrupt(bool enable) {
  if (enable) USARTC0.CTRLA |= USART_DREINTLVL_HI_gc;
  else USARTC0.CTRLA &= ~USART_DREINTLVL_HI_gc;
}


static void _set_rxc_interrupt(bool enable) {
  if (enable) {
    USARTC0.CTRLA |= USART_RXCINTLVL_HI_gc;
    if (4 <= rx_buf_space()) PORTC.OUTCLR = 1 << 4; // CTS Lo (enable)

  } else USARTC0.CTRLA &= ~USART_RXCINTLVL_HI_gc;
}


static void _echo_char(char c) {
  if (echo_buf_full()) return;

  echo_buf_push(c);
  _set_dre_interrupt(true); // Enable interrupt

  if ((usart_flags & USART_CRLF) && c == '\n') _echo_char('\r');
}


// Data register empty interrupt vector
ISR(USARTC0_DRE_vect) {
  if (tx_buf_empty() && echo_buf_empty())
    _set_dre_interrupt(false); // Disable interrupt

  else if (!echo_buf_empty()) {
    USARTC0.DATA = echo_buf_peek();
    echo_buf_pop();

  } else {
    USARTC0.DATA = tx_buf_peek();
    tx_buf_pop();
  }
}


// Data received interrupt vector
ISR(USARTC0_RXC_vect) {
  if (rx_buf_full()) _set_rxc_interrupt(false); // Disable interrupt

  else {
    uint8_t data = USARTC0.DATA;
    rx_buf_push(data);
    if (usart_flags & USART_ECHO) _echo_char(data);
    if (rx_buf_space() < 4) PORTC.OUTSET = 1 << 4; // CTS Hi (disable)
  }
}


static int _usart_putchar(char c, FILE *f) {
  usart_putc(c);
  return 0;
}

static FILE _stdout = FDEV_SETUP_STREAM(_usart_putchar, 0, _FDEV_SETUP_WRITE);


void usart_init(void) {
  // Setup ring buffer
  tx_buf_init();
  rx_buf_init();

  PR.PRPC &= ~PR_USART0_bm; // Disable power reduction

  // Setup pins
  PORTC.OUTSET = 1 << 4; // CTS Hi (disable)
  PORTC.DIRSET = 1 << 4; // CTS Output
  PORTC.OUTSET = 1 << 3; // Tx High
  PORTC.DIRSET = 1 << 3; // Tx Output
  PORTC.DIRCLR = 1 << 2; // Rx Input

  // Set baud rate
  usart_set_baud(USART_BAUD_115200);

  // No parity, 8 data bits, 1 stop bit
  USARTC0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc |
    USART_CHSIZE_8BIT_gc;

  // Configure receiver and transmitter
  USARTC0.CTRLB = USART_RXEN_bm | USART_TXEN_bm | USART_CLK2X_bm;

  PMIC.CTRL |= PMIC_HILVLEN_bm; // Interrupt level on

  // Connect IO
  stdout = &_stdout;
  stderr = &_stdout;

  // Enable Rx
  _set_rxc_interrupt(true);
}


static void _set_baud(uint16_t bsel, uint8_t bscale) {
  USARTC0.BAUDCTRLB = (uint8_t)((bscale << 4) | (bsel >> 8));
  USARTC0.BAUDCTRLA = bsel;
}


void usart_set_baud(int baud) {
  // The BSEL / BSCALE values provided below assume a 32 Mhz clock
  // Assumes CTRLB CLK2X bit (0x04) is set
  // See http://www.avrcalc.elektronik-projekt.de/xmega/baud_rate_calculator

  switch (baud) {
  case USART_BAUD_9600:    _set_baud(3325, 0b1101); break;
  case USART_BAUD_19200:   _set_baud(3317, 0b1100); break;
  case USART_BAUD_38400:   _set_baud(3301, 0b1011); break;
  case USART_BAUD_57600:   _set_baud(1095, 0b1100); break;
  case USART_BAUD_115200:  _set_baud(1079, 0b1011); break;
  case USART_BAUD_230400:  _set_baud(1047, 0b1010); break;
  case USART_BAUD_460800:  _set_baud(983,  0b1001); break;
  case USART_BAUD_921600:  _set_baud(107,  0b1011); break;
  case USART_BAUD_500000:  _set_baud(1,    0b0010); break;
  case USART_BAUD_1000000: _set_baud(1,    0b0001); break;
  }
}


void usart_set(int flag, bool enable) {
  if (enable) usart_flags |= flag;
  else usart_flags &= ~flag;
}


bool usart_is_set(int flags) {
  return (usart_flags & flags) == flags;
}


void usart_putc(char c) {
  while (tx_buf_full() || (usart_flags & USART_FLUSH)) continue;

  tx_buf_push(c);

  _set_dre_interrupt(true); // Enable interrupt

  if ((usart_flags & USART_CRLF) && c == '\n') usart_putc('\r');
}


void usart_puts(const char *s) {
  while (*s) usart_putc(*s++);
}


int8_t usart_getc() {
  while (rx_buf_empty()) continue;

  uint8_t data = rx_buf_peek();
  rx_buf_pop();

  _set_rxc_interrupt(true); // Enable interrupt

  return data;
}


char *usart_readline() {
  static char line[INPUT_BUFFER_LEN];
  static int i = 0;
  bool eol = false;

  while (!rx_buf_empty()) {
    char data = rx_buf_peek();
    rx_buf_pop();

    switch (data) {
    case '\r': case '\n': eol = true; break;

    case '\b':
      printf(" \b");
      if (i) i--;
      break;

    default:
      line[i++] = data;
      if (i == INPUT_BUFFER_LEN - 1) eol = true;
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


int16_t usart_peek() {
  return rx_buf_empty() ? -1 : rx_buf_peek();
}


void usart_flush() {
  usart_set(USART_FLUSH, true);

  while (!tx_buf_empty() || !(USARTC0.STATUS & USART_DREIF_bm) ||
         !(USARTC0.STATUS & USART_TXCIF_bm))
    continue;
}


void usart_rx_flush() {
  rx_buf_init();
}


int16_t usart_rx_space() {
  return rx_buf_space();
}


int16_t usart_rx_fill() {
  return rx_buf_fill();
}


int16_t usart_tx_space() {
  return tx_buf_space();
}


int16_t usart_tx_fill() {
  return tx_buf_fill();
}
