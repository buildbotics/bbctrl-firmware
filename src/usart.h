/******************************************************************************\

                   This file is part of the TinyG firmware.

                   Copyright (c) 2015-2016, Buildbotics LLC
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

#ifndef USART_H
#define USART_H

#include <stdint.h>

#define USART_TX_RING_BUF_SIZE 256
#define USART_RX_RING_BUF_SIZE 256

enum {
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
};

enum {
  USART_CRLF = (1 << 0),
  USART_ECHO = (1 << 1),
  USART_XOFF = (1 << 2),
};

void usart_init();
void usart_set_baud(int baud);
void usart_ctrl(int flag, int enable);
void usart_putc(char c);
void usart_puts(const char *s);
int8_t usart_getc();
int usart_gets(char *buf, int size);
int16_t usart_peek();

void usart_rx_flush();
int16_t usart_rx_fill();
int16_t usart_rx_space();
inline int usart_rx_empty() {return !usart_rx_fill();}
inline int usart_rx_full() {return !usart_rx_space();}

int16_t usart_tx_fill();
int16_t usart_tx_space();
inline int usart_tx_empty() {return !usart_tx_fill();}
inline int usart_tx_full() {return !usart_tx_space();}

#endif // USART_H
