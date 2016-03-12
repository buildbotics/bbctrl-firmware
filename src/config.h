#pragma once

#include <avr/interrupt.h>


// Compile-time settings
#define __STEP_CORRECTION
//#define __JERK_EXEC            // Use computed jerk (vs. forward difference)
//#define __KAHAN                // Use Kahan summation in aline exec functions


#define INPUT_BUFFER_LEN 255     // text buffer size (255 max)

#define AXES         6           // number of axes
#define MOTORS       4           // number of motors on the board
#define COORDS       6           // number of supported coordinate systems (1-6)
#define PWMS         2           // number of supported PWM channels

// Motor settings
#define STEP_CLOCK_FREQ 25000
#define MOTOR_CURRENT            0.8    // 1.0 is full power
#define MOTOR_MICROSTEPS         16

/// One of: MOTOR_DISABLED, MOTOR_ALWAYS_POWERED, MOTOR_POWERED_IN_CYCLE,
///   MOTOR_POWERED_ONLY_WHEN_MOVING
#define MOTOR_POWER_MODE         MOTOR_ALWAYS_POWERED

/// Seconds to maintain motor at full power before idling
#define MOTOR_IDLE_TIMEOUT       2.00

#define MAX_VELOCITY(angle, travel, mstep) \
  (0.98 * (angle) * (travel) * STEP_CLOCK_FREQ / (mstep) / 6.0)

#define M1_MOTOR_MAP             AXIS_X
#define M1_STEP_ANGLE            1.8
#define M1_TRAVEL_PER_REV        1.25
#define M1_MICROSTEPS            MOTOR_MICROSTEPS
#define M1_POLARITY              0                   // 0=normal, 1=reversed
#define M1_POWER_MODE            MOTOR_POWER_MODE
#define M1_POWER_LEVEL           MOTOR_POWER_LEVEL

#define M2_MOTOR_MAP             AXIS_Y
#define M2_STEP_ANGLE            1.8
#define M2_TRAVEL_PER_REV        1.25
#define M2_MICROSTEPS            MOTOR_MICROSTEPS
#define M2_POLARITY              0
#define M2_POWER_MODE            MOTOR_POWER_MODE
#define M2_POWER_LEVEL           MOTOR_POWER_LEVEL

#define M3_MOTOR_MAP             AXIS_Z
#define M3_STEP_ANGLE            1.8
#define M3_TRAVEL_PER_REV        1.25
#define M3_MICROSTEPS            MOTOR_MICROSTEPS
#define M3_POLARITY              0
#define M3_POWER_MODE            MOTOR_POWER_MODE
#define M3_POWER_LEVEL           MOTOR_POWER_LEVEL

#define M4_MOTOR_MAP             AXIS_A
#define M4_STEP_ANGLE            1.8
#define M4_TRAVEL_PER_REV        360            // degrees moved per motor rev
#define M4_MICROSTEPS            MOTOR_MICROSTEPS
#define M4_POLARITY              0
#define M4_POWER_MODE            MOTOR_POWER_MODE
#define M4_POWER_LEVEL           MOTOR_POWER_LEVEL

#define M5_MOTOR_MAP             AXIS_B
#define M5_STEP_ANGLE            1.8
#define M5_TRAVEL_PER_REV        360
#define M5_MICROSTEPS            MOTOR_MICROSTEPS
#define M5_POLARITY              0
#define M5_POWER_MODE            MOTOR_POWER_MODE
#define M5_POWER_LEVEL           MOTOR_POWER_LEVEL

#define M6_MOTOR_MAP             AXIS_C
#define M6_STEP_ANGLE            1.8
#define M6_TRAVEL_PER_REV        360
#define M6_MICROSTEPS            MOTOR_MICROSTEPS
#define M6_POLARITY              0
#define M6_POWER_MODE            MOTOR_POWER_MODE
#define M6_POWER_LEVEL           MOTOR_POWER_LEVEL


// Switch settings
/// one of: SW_TYPE_NORMALLY_OPEN, SW_TYPE_NORMALLY_CLOSED
#define SWITCH_TYPE              SW_TYPE_NORMALLY_OPEN
/// SW_MODE_DISABLED, SW_MODE_HOMING, SW_MODE_LIMIT, SW_MODE_HOMING_LIMIT
#define X_SWITCH_MODE_MIN        SW_MODE_HOMING
#define X_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define Y_SWITCH_MODE_MIN        SW_MODE_HOMING
#define Y_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define Z_SWITCH_MODE_MIN        SW_MODE_DISABLED
#define Z_SWITCH_MODE_MAX        SW_MODE_HOMING
#define A_SWITCH_MODE_MIN        SW_MODE_HOMING
#define A_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define B_SWITCH_MODE_MIN        SW_MODE_HOMING
#define B_SWITCH_MODE_MAX        SW_MODE_DISABLED
#define C_SWITCH_MODE_MIN        SW_MODE_HOMING
#define C_SWITCH_MODE_MAX        SW_MODE_DISABLED

// Jog settings
#define JOG_ACCELERATION          50000  // mm/min^2

// Machine settings
#define CHORDAL_TOLERANCE         0.01   // chordal accuracy for arc drawing
#define SOFT_LIMIT_ENABLE         0      // 0 = off, 1 = on
#define JERK_MAX                  10     // yes, that's "20,000,000" mm/min^3
#define JUNCTION_DEVIATION        0.05   // default value, in mm
#define JUNCTION_ACCELERATION     100000 // centripetal corner acceleration


// Axis settings
#define VELOCITY_MAX                                                \
  MAX_VELOCITY(M1_STEP_ANGLE, M1_TRAVEL_PER_REV, MOTOR_MICROSTEPS)
#define FEEDRATE_MAX             VELOCITY_MAX

// see canonical_machine.h cmAxisMode for valid values
#define X_AXIS_MODE              AXIS_STANDARD
#define X_VELOCITY_MAX           VELOCITY_MAX  // G0 max velocity in mm/min
#define X_FEEDRATE_MAX           FEEDRATE_MAX  // G1 max feed rate in mm/min
#define X_TRAVEL_MIN             0             // minimum travel for soft limits
#define X_TRAVEL_MAX             150           // between switches or crashes
#define X_JERK_MAX               JERK_MAX
#define X_JERK_HOMING            (X_JERK_MAX * 2)
#define X_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define X_SEARCH_VELOCITY        500           // move in negative direction
#define X_LATCH_VELOCITY         100           // mm/min
#define X_LATCH_BACKOFF          5             // mm
#define X_ZERO_BACKOFF           1             // mm

#define Y_AXIS_MODE              AXIS_STANDARD
#define Y_VELOCITY_MAX           VELOCITY_MAX
#define Y_FEEDRATE_MAX           FEEDRATE_MAX
#define Y_TRAVEL_MIN             0
#define Y_TRAVEL_MAX             150
#define Y_JERK_MAX               JERK_MAX
#define Y_JERK_HOMING            (Y_JERK_MAX * 2)
#define Y_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define Y_SEARCH_VELOCITY        500
#define Y_LATCH_VELOCITY         100
#define Y_LATCH_BACKOFF          5
#define Y_ZERO_BACKOFF           1

#define Z_AXIS_MODE              AXIS_STANDARD
#define Z_VELOCITY_MAX           VELOCITY_MAX
#define Z_FEEDRATE_MAX           FEEDRATE_MAX
#define Z_TRAVEL_MIN             0
#define Z_TRAVEL_MAX             75
#define Z_JERK_MAX               JERK_MAX
#define Z_JERK_HOMING            (Z_JERK_MAX * 2)
#define Z_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define Z_SEARCH_VELOCITY        400
#define Z_LATCH_VELOCITY         100
#define Z_LATCH_BACKOFF          5
#define Z_ZERO_BACKOFF           1

// A values are chosen to make the A motor react the same as X for testing
// set to the same speed as X axis
#define A_AXIS_MODE              AXIS_RADIUS
#define A_VELOCITY_MAX           ((X_VELOCITY_MAX / M1_TRAVEL_PER_REV) * 360)
#define A_FEEDRATE_MAX           A_VELOCITY_MAX
#define A_TRAVEL_MIN             -1
#define A_TRAVEL_MAX             -1 // same number means infinite
#define A_JERK_MAX               (X_JERK_MAX * (360 / M1_TRAVEL_PER_REV))
#define A_JERK_HOMING            (A_JERK_MAX * 2)
#define A_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define A_RADIUS                 (M1_TRAVEL_PER_REV/(2*3.14159628))
#define A_SEARCH_VELOCITY         600
#define A_LATCH_VELOCITY          100
#define A_LATCH_BACKOFF           5
#define A_ZERO_BACKOFF            2

#define B_AXIS_MODE               AXIS_DISABLED
#define B_VELOCITY_MAX            3600
#define B_FEEDRATE_MAX            B_VELOCITY_MAX
#define B_TRAVEL_MIN              -1
#define B_TRAVEL_MAX              -1
#define B_JERK_MAX                JERK_MAX
#define B_JERK_HOMING             (B_JERK_MAX * 2)
#define B_JUNCTION_DEVIATION      JUNCTION_DEVIATION
#define B_RADIUS                  1
#define B_SEARCH_VELOCITY         600
#define B_LATCH_VELOCITY          100
#define B_LATCH_BACKOFF           5
#define B_ZERO_BACKOFF            2

#define C_AXIS_MODE               AXIS_DISABLED
#define C_VELOCITY_MAX            3600
#define C_FEEDRATE_MAX            C_VELOCITY_MAX
#define C_TRAVEL_MIN              -1
#define C_TRAVEL_MAX              -1
#define C_JERK_MAX                JERK_MAX
#define C_JERK_HOMING             (C_JERK_MAX * 2)
#define C_JUNCTION_DEVIATION      JUNCTION_DEVIATION
#define C_RADIUS                  1
#define C_SEARCH_VELOCITY         600
#define C_LATCH_VELOCITY          100
#define C_LATCH_BACKOFF           5
#define C_ZERO_BACKOFF            2


// PWM settings
#define P1_PWM_FREQUENCY          100    // in Hz
#define P1_CW_SPEED_LO            1000   // in RPM (arbitrary units)
#define P1_CW_SPEED_HI            2000
#define P1_CW_PHASE_LO            0.125  // phase [0..1]
#define P1_CW_PHASE_HI            0.2
#define P1_CCW_SPEED_LO           1000
#define P1_CCW_SPEED_HI           2000
#define P1_CCW_PHASE_LO           0.125
#define P1_CCW_PHASE_HI           0.2
#define P1_PWM_PHASE_OFF          0.1


// Gcode defaults
#define GCODE_DEFAULT_UNITS         MILLIMETERS // MILLIMETERS or INCHES
// CANON_PLANE_XY, CANON_PLANE_XZ, or CANON_PLANE_YZ
#define GCODE_DEFAULT_PLANE         CANON_PLANE_XY
#define GCODE_DEFAULT_COORD_SYSTEM  G54 // G54, G55, G56, G57, G58 or G59
#define GCODE_DEFAULT_PATH_CONTROL  PATH_CONTINUOUS
#define GCODE_DEFAULT_DISTANCE_MODE ABSOLUTE_MODE

#define AXIS_X       0
#define AXIS_Y       1
#define AXIS_Z       2
#define AXIS_A       3
#define AXIS_B       4
#define AXIS_C       5
#define AXIS_U       6           // reserved
#define AXIS_V       7           // reserved
#define AXIS_W       8           // reserved


#define MILLISECONDS_PER_TICK 1  // MS for system tick (systick * N)
#define SYS_ID_LEN 12            // length of system ID string from sys_get_id()

// Clock Crystal Config. Pick one:
//#define __CLOCK_INTERNAL_32MHZ TRUE // use internal oscillator
//#define __CLOCK_EXTERNAL_8MHZ TRUE  // uses PLL to provide 32 MHz system clock
#define __CLOCK_EXTERNAL_16MHZ TRUE   // uses PLL to provide 32 MHz system clock

// Motor, output bit & switch port assignments
// These are not all the same, and must line up in multiple places in gpio.h
// Sorry if this is confusing - it's a board routing issue
#define PORT_MOTOR_1     PORTA        // motors mapped to ports
#define PORT_MOTOR_2     PORTF
#define PORT_MOTOR_3     PORTE
#define PORT_MOTOR_4     PORTD

#define PORT_SWITCH_X    PORTA        // Switch axes mapped to ports
#define PORT_SWITCH_Y    PORTD
#define PORT_SWITCH_Z    PORTE
#define PORT_SWITCH_A    PORTF

#define PORT_OUT_V7_X    PORTA        // v7 mapping
#define PORT_OUT_V7_Y    PORTF
#define PORT_OUT_V7_Z    PORTD
#define PORT_OUT_V7_A    PORTE

// These next four must be changed when the PORT_MOTOR_* definitions change!
#define PORTCFG_VP0MAP_PORT_MOTOR_1_gc PORTCFG_VP02MAP_PORTA_gc
#define PORTCFG_VP1MAP_PORT_MOTOR_2_gc PORTCFG_VP13MAP_PORTF_gc
#define PORTCFG_VP2MAP_PORT_MOTOR_3_gc PORTCFG_VP02MAP_PORTE_gc
#define PORTCFG_VP3MAP_PORT_MOTOR_4_gc PORTCFG_VP13MAP_PORTD_gc

#define PORT_MOTOR_1_VPORT VPORT0
#define PORT_MOTOR_2_VPORT VPORT1
#define PORT_MOTOR_3_VPORT VPORT2
#define PORT_MOTOR_4_VPORT VPORT3

/*
 * Port setup - Stepper / Switch Ports:
 *    b0    (out) step             (SET is step,  CLR is rest)
 *    b1    (out) direction        (CLR = Clockwise)
 *    b2    (out) motor enable     (CLR = Enabled)
 *    b3    (out) chip select
 *    b4    (in)  fault
 *    b5    (out) output bit for GPIO port1
 *    b6    (in)  min limit switch on GPIO 2 *
 *    b7    (in)  max limit switch on GPIO 2 *
 *  * motor controls and GPIO2 port mappings are not the same
 */
#define MOTOR_PORT_DIR_gm 0x2f // pin dir settings

enum cfgPortBits {        // motor control port bit positions
  STEP_BIT_bp = 0,        // bit 0
  DIRECTION_BIT_bp,       // bit 1
  MOTOR_ENABLE_BIT_bp,    // bit 2
  CHIP_SELECT_BIT_bp,     // bit 3
  FAULT_BIT_bp,           // bit 4
  GPIO1_OUT_BIT_bp,       // bit 5 (4 gpio1 output bits; 1 from each axis)
  SW_MIN_BIT_bp,          // bit 6 (4 input bits for homing/limit switches)
  SW_MAX_BIT_bp           // bit 7 (4 input bits for homing/limit switches)
};

#define STEP_BIT_bm         (1 << STEP_BIT_bp)
#define DIRECTION_BIT_bm    (1 << DIRECTION_BIT_bp)
#define MOTOR_ENABLE_BIT_bm (1 << MOTOR_ENABLE_BIT_bp)
#define CHIP_SELECT_BIT_bm  (1 << CHIP_SELECT_BIT_bp)
#define FAULT_BIT_bm        (1 << FAULT_BIT_bp)
#define GPIO1_OUT_BIT_bm    (1 << GPIO1_OUT_BIT_bp) // spindle and coolant
#define SW_MIN_BIT_bm       (1 << SW_MIN_BIT_bp)    // minimum switch inputs
#define SW_MAX_BIT_bm       (1 << SW_MAX_BIT_bp)    // maximum switch inputs

// Bit assignments for GPIO1_OUTs for spindle, PWM and coolant
#define SPINDLE_BIT         8 // spindle on/off
#define SPINDLE_DIR         4 // spindle direction, 1=CW, 0=CCW
#define SPINDLE_PWM         2 // spindle PWMs output bit
#define MIST_COOLANT_BIT    1 // coolant on/off (same as flood)
#define FLOOD_COOLANT_BIT   1 // coolant on/off (same as mist)

#define SPINDLE_LED         0
#define SPINDLE_DIR_LED     1
#define SPINDLE_PWM_LED     2
#define COOLANT_LED         3

// Can use the spindle direction as an indicator LED
#define INDICATOR_LED       SPINDLE_DIR_LED

/*
 * Interrupt usage - TinyG uses a lot of them all over the place
 *
 *    HI    Stepper DDA pulse generation         (set in stepper.h)
 *    HI    Stepper load routine SW interrupt    (set in stepper.h)
 *    HI    Dwell timer counter                  (set in stepper.h)
 *    LO    Segment execution SW interrupt       (set in stepper.h)
 *   MED    GPIO1 switch port                    (set in gpio.h)
 *   MED    Serial RX                            (set in usart.c)
 *   MED    Serial TX                            (set in usart.c) (* see note)
 *    LO    Real time clock interrupt            (set in xmega_rtc.h)
 *
 *    (*) The TX cannot run at LO level or exception reports and other prints
 *        called from a LO interrupt (as in prep_line()) will kill the system
 *        in a permanent sleep_mode() call in usart_putc() (usart.c) as no
 *        interrupt can release the sleep mode.
 */

// Timer assignments - see specific modules for details
#define TIMER_DDA           TCC0 // DDA timer       (see stepper.h)
#define TIMER_TMC2660       TCC1 // TMC2660 timer   (see tmc2660.h)
#define TIMER_PWM1          TCD1 // PWM timer #1    (see pwm.c)
#define TIMER_PWM2          TCD1 // PWM timer #2    (see pwm.c)
#define TIMER_MOTOR1        TCE1
#define TIMER_MOTOR2        TCF0
#define TIMER_MOTOR3        TCE0
#define TIMER_MOTOR4        TCD0

// Timer setup for stepper and dwells
#define FREQUENCY_DDA        STEP_CLOCK_FREQ // DDA frequency in hz.
#define STEP_TIMER_DISABLE   0     // turn timer off
#define STEP_TIMER_ENABLE    1     // turn timer clock on
#define STEP_TIMER_WGMODE    0     // normal mode (count to TOP and rollover)
#define TIMER_DDA_ISR_vect   TCC0_OVF_vect
#define TIMER_DDA_INTLVL     3     // Timer overflow HI

#define PWM1_CTRLB           (3 | TC1_CCBEN_bm) // single slope PWM channel B
#define PWM1_ISR_vect        TCD1_CCB_vect
#define PWM2_CTRLA_CLKSEL    TC_CLKSEL_DIV1_gc
#define PWM2_CTRLB           3                  // single slope PWM no output
#define PWM2_ISR_vect        TCE1_CCB_vect
