/******************************************************************************\

                This file is part of the Buildbotics firmware.

                  Copyright (c) 2015 - 2016 Buildbotics LLC
                  Copyright (c) 2010 - 2015 Alden S. Hart, Jr.
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

/* Switch processing functions
 *
 * Switch processing turns pin transitions into reliable switch states.
 * There are 2 main operations:
 *
 *   - read pin        get raw data from a pin
 *   - read switch        return processed switch closures
 *
 * Read pin may be a polled operation or an interrupt on pin
 * change. If interrupts are used they must be provided for both
 * leading and trailing edge transitions.
 *
 * Read switch contains the results of read pin and manages edges and
 * debouncing.
 */
#pragma once


#include <stdint.h>

// timer for debouncing switches
#define SW_LOCKOUT_TICKS 25          // 25=250ms. RTC ticks are ~10ms each
#define SW_DEGLITCH_TICKS 3          // 3=30ms

// switch modes
enum {
  SW_MODE_DISABLED,
  SW_HOMING_BIT,
  SW_LIMIT_BIT
};

#define SW_MODE_HOMING        SW_HOMING_BIT   // enable switch for homing only
#define SW_MODE_LIMIT         SW_LIMIT_BIT    // enable switch for limits only
#define SW_MODE_HOMING_LIMIT  (SW_HOMING_BIT | SW_LIMIT_BIT) // homing & limits

enum swType {
  SW_TYPE_NORMALLY_OPEN,
  SW_TYPE_NORMALLY_CLOSED
};

enum swState {
  SW_DISABLED = -1,
  SW_OPEN,
  SW_CLOSED
};

// macros for finding the index into the switch table give the axis number
#define MIN_SWITCH(axis) (axis * 2)
#define MAX_SWITCH(axis) (axis * 2 + 1)

enum swDebounce { // state machine for managing debouncing and lockout
  SW_IDLE,
  SW_DEGLITCHING,
  SW_LOCKOUT
};

enum swNums { // indexes into switch arrays
  SW_MIN_X,
  SW_MAX_X,
  SW_MIN_Y,
  SW_MAX_Y,
  SW_MIN_Z,
  SW_MAX_Z,
  SW_MIN_A,
  SW_MAX_A,
  NUM_SWITCHES // must be last one. Used for array sizing and for loops
};
#define SW_OFFSET SW_MAX_X // offset between MIN and MAX switches
#define NUM_SWITCH_PAIRS (NUM_SWITCHES / 2)

/// Interrupt levels and vectors - The vectors are hard-wired to xmega ports
/// If you change axis port assignments you need to change these, too.
#define GPIO1_INTLVL (PORT_INT0LVL_MED_gc | PORT_INT1LVL_MED_gc)

// port assignments for vectors
#define X_MIN_ISR_vect PORTA_INT0_vect
#define Y_MIN_ISR_vect PORTD_INT0_vect
#define Z_MIN_ISR_vect PORTE_INT0_vect
#define A_MIN_ISR_vect PORTF_INT0_vect
#define X_MAX_ISR_vect PORTA_INT1_vect
#define Y_MAX_ISR_vect PORTD_INT1_vect
#define Z_MAX_ISR_vect PORTE_INT1_vect
#define A_MAX_ISR_vect PORTF_INT1_vect

/* Switch control structures
 * Note 1: The term "thrown" is used because switches could be normally-open
 * or normally-closed. "Thrown" means activated or hit.
 */
struct swStruct {                       // switch state
  uint8_t switch_type;                  // 0=NO, 1=NC - applies to all switches
  uint8_t limit_flag;                   // 1=limit switch thrown - do a lockout
  uint8_t sw_num_thrown;                // number of switch that was just thrown
  /// 0=OPEN, 1=CLOSED (depends on switch type)
  uint8_t state[NUM_SWITCHES];
  /// 0=disabled, 1=homing, 2=homing+limit, 3=limit
  volatile uint8_t mode[NUM_SWITCHES];
  /// debouncer state machine - see swDebounce
  volatile uint8_t debounce[NUM_SWITCHES];
  volatile int8_t count[NUM_SWITCHES];  // deglitching and lockout counter
};
struct swStruct sw;


void switch_init();
uint8_t read_switch(uint8_t sw_num);
uint8_t get_switch_mode(uint8_t sw_num);

void switch_rtc_callback();
uint8_t get_limit_switch_thrown();
uint8_t get_switch_thrown();
void reset_switches();
void sw_show_switch();

void set_switch_type(uint8_t switch_type);
uint8_t get_switch_type();


