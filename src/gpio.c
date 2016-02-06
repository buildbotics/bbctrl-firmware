/*
 * gpio.c - general purpose IO bits
 * Part of TinyG project
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart Jr.
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software library without
 * restriction. Specifically, if other files instantiate templates or use macros or
 * inline functions from this file, or you compile this file and link it with  other
 * files to produce an executable, this file does not by itself cause the resulting
 * executable to be covered by the GNU General Public License. This exception does not
 * however invalidate any other reasons why the executable file might be covered by the
 * GNU General Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 *    This GPIO file is where all parallel port bits are managed that are
 *    not already taken up by steppers, serial ports, SPI or PDI programming
 *
 *    There are 2 GPIO ports:
 *
 *      gpio1   Located on 5x2 header next to the PDI programming plugs (on v7's)
 *              Four (4) output bits capable of driving 3.3v or 5v logic
 *
 *              Note: On v6 and earlier boards there are also 4 inputs:
 *              Four (4) level converted input bits capable of being driven
 *              by 3.3v or 5v logic - connected to B0 - B3 (now used for SPI)
 *
 *      gpio2   Located on 9x2 header on "bottom" edge of the board
 *              Eight (8) non-level converted input bits
 *              Eight (8) ground pins - one each "under" each input pin
 *              Two   (2) 3.3v power pins (on left edge of connector)
 *              Inputs can be used as switch contact inputs or
 *              3.3v input bits depending on port configuration
 *              **** These bits CANNOT be used as 5v inputs ****
 */

#include "controller.h"
#include "hardware.h"
#include "gpio.h"

#include <avr/interrupt.h>


void indicator_led_set() {
  gpio_led_on(INDICATOR_LED);
}


void indicator_led_clear() {
  gpio_led_off(INDICATOR_LED);
}


void indicator_led_toggle() {
  gpio_led_toggle(INDICATOR_LED);
}


void gpio_led_on(uint8_t led) {
  gpio_set_bit_on(8 >> led);
}


void gpio_led_off(uint8_t led) {
  gpio_set_bit_off(8 >> led);
}


void gpio_led_toggle(uint8_t led) {
  gpio_set_bit_toggle(8 >> led);
}


uint8_t gpio_read_bit(uint8_t b) {
  if (b & 0x08) return hw.out_port[0]->IN & GPIO1_OUT_BIT_bm;
  if (b & 0x04) return hw.out_port[1]->IN & GPIO1_OUT_BIT_bm;
  if (b & 0x02) return hw.out_port[2]->IN & GPIO1_OUT_BIT_bm;
  if (b & 0x01) return hw.out_port[3]->IN & GPIO1_OUT_BIT_bm;

  return 0;
}


void gpio_set_bit_on(uint8_t b) {
  if (b & 0x08) hw.out_port[0]->OUTSET = GPIO1_OUT_BIT_bm;
  if (b & 0x04) hw.out_port[1]->OUTSET = GPIO1_OUT_BIT_bm;
  if (b & 0x02) hw.out_port[2]->OUTSET = GPIO1_OUT_BIT_bm;
  if (b & 0x01) hw.out_port[3]->OUTSET = GPIO1_OUT_BIT_bm;
}


void gpio_set_bit_off(uint8_t b) {
  if (b & 0x08) hw.out_port[0]->OUTCLR = GPIO1_OUT_BIT_bm;
  if (b & 0x04) hw.out_port[1]->OUTCLR = GPIO1_OUT_BIT_bm;
  if (b & 0x02) hw.out_port[2]->OUTCLR = GPIO1_OUT_BIT_bm;
  if (b & 0x01) hw.out_port[3]->OUTCLR = GPIO1_OUT_BIT_bm;
}


void gpio_set_bit_toggle(uint8_t b) {
  if (b & 0x08) hw.out_port[0]->OUTTGL = GPIO1_OUT_BIT_bm;
  if (b & 0x04) hw.out_port[1]->OUTTGL = GPIO1_OUT_BIT_bm;
  if (b & 0x02) hw.out_port[2]->OUTTGL = GPIO1_OUT_BIT_bm;
  if (b & 0x01) hw.out_port[3]->OUTTGL = GPIO1_OUT_BIT_bm;
}
