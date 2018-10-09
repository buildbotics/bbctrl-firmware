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

#pragma once

#include <avr/io.h>

#include <stdint.h>
#include <stdbool.h>


#define USART_TX_RING_BUF_SIZE 256
#define USART_RX_RING_BUF_SIZE 256


typedef enum {
  USART_BAUD_9600,
  USART_BAUD_19200,
  USART_BAUD_38400,
  USART_BAUD_57600,
  USART_BAUD_115200,
  USART_BAUD_230400,
  USART_BAUD_460800,
  USART_BAUD_921600,
  USART_BAUD_500000,
  USART_BAUD_1000000
} baud_t;


typedef enum {
  USART_NONE,
  USART_EVEN,
  USART_ODD,
} parity_t;


typedef enum {
  USART_1STOP,
  USART_2STOP,
} stop_t;


typedef enum {
  USART_5BITS,
  USART_6BITS,
  USART_7BITS,
  USART_8BITS,
  USART_9BITS,
} bits_t;


void usart_set_baud(USART_t *port, baud_t baud);
void usart_set_parity(USART_t *port, parity_t parity);
void usart_set_stop(USART_t *port, stop_t stop);
void usart_set_bits(USART_t *port, bits_t bits);
void usart_init_port(USART_t *port, baud_t baud, parity_t parity, bits_t bits,
                     stop_t stop);

void usart_init();
void usart_putc(char c);
void usart_puts(const char *s);
int8_t usart_getc();
char *usart_readline();
void usart_flush();

void usart_rx_flush();
int16_t usart_rx_fill();
int16_t usart_rx_space();
inline bool usart_rx_empty() {return !usart_rx_fill();}
inline bool usart_rx_full() {return !usart_rx_space();}

int16_t usart_tx_fill();
int16_t usart_tx_space();
inline bool usart_tx_empty() {return !usart_tx_fill();}
inline bool usart_tx_full() {return !usart_tx_space();}
