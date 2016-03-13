/*
 * hardware.h - system hardware configuration
 * This file is hardware platform specific - AVR Xmega version
 *
 * This file is part of the TinyG project
 *
 * Copyright (c) 2013 - 2015 Alden S. Hart, Jr.
 * Copyright (c) 2013 - 2015 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License, version 2 as published by the Free Software
 * Foundation. You should have received a copy of the GNU General
 * Public License, version 2 along with the software.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * As a special exception, you may use this file as part of a software
 * library without restriction. Specifically, if other files
 * instantiate templates or use macros or inline functions from this
 * file, or you compile this file and link it with  other files to
 * produce an executable, this file does not by itself cause the
 * resulting executable to be covered by the GNU General Public
 * License. This exception does not however invalidate any other
 * reasons why the executable file might be covered by the GNU General
 * Public License.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT
 * WITHOUT ANY WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "status.h"
#include "config.h"

/*
  Device singleton - global structure to allow iteration through similar devices

  Ports are shared between steppers and GPIO so we need a global struct.
  Each xmega port has 3 bindings; motors, switches and the output bit

  Care needs to be taken in routines that use ports not to write to bits that
  are not assigned to the designated function - ur unpredicatable results will
  occur.
*/
typedef struct {
  PORT_t *st_port[MOTORS];  // bindings for stepper motor ports (stepper.c)
  PORT_t *sw_port[MOTORS];  // bindings for switch ports (GPIO2)
  PORT_t *out_port[MOTORS]; // bindings for output ports (GPIO1)
} hwSingleton_t;
extern hwSingleton_t hw;


void hardware_init();
void hw_get_id(char *id);
void hw_request_hard_reset();
void hw_hard_reset();
stat_t hw_hard_reset_handler();

void hw_request_bootloader();
stat_t hw_bootloader_handler();
