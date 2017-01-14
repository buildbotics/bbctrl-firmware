/************************************************************************/
/* XBoot Extensible AVR Bootloader                                      */
/*                                                                      */
/* UART Module                                                          */
/*                                                                      */
/* uart.c                                                               */
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

#include "uart.h"
#include "xboot.h"


bool uart_has_char() {return UART_DEVICE.STATUS & USART_RXCIF_bm;}
uint8_t uart_recv_char() {return UART_DEVICE.DATA;}


void uart_send_char_blocking(uint8_t c) {
  UART_DEVICE.DATA = c;
  while (!(UART_DEVICE.STATUS & USART_TXCIF_bm)) continue;
  UART_DEVICE.STATUS |= USART_TXCIF_bm;
}


void uart_init() {
  UART_PORT.DIRSET = 1 << UART_TX_PIN;
  UART_DEVICE.BAUDCTRLA = UART_BSEL_VALUE & USART_BSEL_gm;
  UART_DEVICE.BAUDCTRLB =
    ((UART_BSCALE_VALUE << USART_BSCALE_gp) & USART_BSCALE_gm) |
    ((UART_BSEL_VALUE >> 8) & ~USART_BSCALE_gm);

#if UART_CLK2X
  UART_DEVICE.CTRLB = USART_RXEN_bm | USART_CLK2X_bm | USART_TXEN_bm;
#else
  UART_DEVICE.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
#endif // UART_CLK2X

  PORTC.OUTCLR = 1 << 4; // CTS Lo (enable)
  PORTC.DIRSET = 1 << 4; // CTS Output
}


void uart_deinit() {
  UART_DEVICE.CTRLB = 0;
  UART_DEVICE.BAUDCTRLA = 0;
  UART_DEVICE.BAUDCTRLB = 0;
  UART_PORT.DIRCLR = 1 << UART_TX_PIN;
}
