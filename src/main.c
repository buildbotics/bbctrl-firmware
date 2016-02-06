/*
 * main.c - TinyG - An embedded rs274/ngc CNC controller
 * This file is part of the TinyG project.
 *
 * Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
 * Copyright (c) 2013 - 2015 Robert Giseburt
 *
 * This file ("the software") is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2 as published by the
 * Free Software Foundation. You should have received a copy of the GNU General Public
 * License, version 2 along with the software.  If not, see <http://www.gnu.org/licenses/>.
 *
 * THE SOFTWARE IS DISTRIBUTED IN THE HOPE THAT IT WILL BE USEFUL, BUT WITHOUT ANY
 * WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
 * SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "hardware.h"
#include "controller.h"
#include "canonical_machine.h"
#include "stepper.h"
#include "encoder.h"
#include "switch.h"
#include "pwm.h"
#include "usart.h"
#include "tmc2660.h"
#include "plan/planner.h"

#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <stdio.h>


static void init() {
  // There are a lot of dependencies in the order of these inits.
  // Don't change the ordering unless you understand this.

  cli();

  // do these first
  hardware_init();                // system hardware setup            - must be first
  usart_init();                   // serial port

  // do these next
  tmc2660_init();                 // motor drivers
  stepper_init();                 // stepper subsystem
  encoder_init();                 // virtual encoders
  switch_init();                  // switches
  pwm_init();                     // pulse width modulation drivers

  controller_init();              // must be first app init
  planner_init();                 // motion planning subsystem
  canonical_machine_init();       // canonical machine                - must follow config_init()

  // set vector location to application, as opposed to boot ROM
  uint8_t temp = PMIC.CTRL & ~PMIC_IVSEL_bm;
  CCP = CCP_IOREG_gc;
  PMIC.CTRL = temp;

  // all levels are used, so don't bother to abstract them
  PMIC.CTRL |= PMIC_HILVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;

  sei();                          // enable global interrupts

  fprintf_P(stderr, PSTR("TinyG firmware\n"));
}


int main() {
  init();

  // main loop
  while (1) controller_run();  // single pass through the controller

  return 0;
}
