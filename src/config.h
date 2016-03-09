#ifndef CONFIG_H
#define CONFIG_H

// Compile-time settings
#define __STEP_CORRECTION
//#define __JERK_EXEC            // Use computed jerk (vs. forward difference)
//#define __KAHAN                // Use Kahan summation in aline exec functions

#define INPUT_BUFFER_LEN 255     // text buffer size (255 max)

#define AXES         6           // number of axes
#define MOTORS       4           // number of motors on the board
#define COORDS       6           // number of supported coordinate systems (1-6)
#define PWMS         2           // number of supported PWM channels

#define AXIS_X       0
#define AXIS_Y       1
#define AXIS_Z       2
#define AXIS_A       3
#define AXIS_B       4
#define AXIS_C       5
#define AXIS_U       6           // reserved
#define AXIS_V       7           // reserved
#define AXIS_W       8           // reserved

// Motor settings
#define MOTOR_MICROSTEPS         8

/// One of:
///   MOTOR_DISABLED
///   MOTOR_ALWAYS_POWERED
///   MOTOR_POWERED_IN_CYCLE
///   MOTOR_POWERED_ONLY_WHEN_MOVING
#define MOTOR_POWER_MODE         MOTOR_ALWAYS_POWERED

/// Seconds to maintain motor at full power before idling
#define MOTOR_IDLE_TIMEOUT       2.00


#define M1_MOTOR_MAP             AXIS_X              // 1ma
#define M1_STEP_ANGLE            1.8                 // 1sa
#define M1_TRAVEL_PER_REV        1.25                // 1tr
#define M1_MICROSTEPS            MOTOR_MICROSTEPS    // 1mi
#define M1_POLARITY              0                   // 1po        0=normal, 1=reversed
#define M1_POWER_MODE            MOTOR_POWER_MODE    // 1pm        standard
#define M1_POWER_LEVEL           MOTOR_POWER_LEVEL   // 1mp

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
#define M5_TRAVEL_PER_REV        360            // degrees moved per motor rev
#define M5_MICROSTEPS            MOTOR_MICROSTEPS
#define M5_POLARITY              0
#define M5_POWER_MODE            MOTOR_POWER_MODE
#define M5_POWER_LEVEL           MOTOR_POWER_LEVEL

#define M6_MOTOR_MAP             AXIS_C
#define M6_STEP_ANGLE            1.8
#define M6_TRAVEL_PER_REV        360            // degrees moved per motor rev
#define M6_MICROSTEPS            MOTOR_MICROSTEPS
#define M6_POLARITY              0
#define M6_POWER_MODE            MOTOR_POWER_MODE
#define M6_POWER_LEVEL           MOTOR_POWER_LEVEL


// Switch settings
#define SWITCH_TYPE              SW_TYPE_NORMALLY_OPEN // one of: SW_TYPE_NORMALLY_OPEN, SW_TYPE_NORMALLY_CLOSED
#define X_SWITCH_MODE_MIN        SW_MODE_HOMING        // xsn  SW_MODE_DISABLED, SW_MODE_HOMING, SW_MODE_LIMIT, SW_MODE_HOMING_LIMIT
#define X_SWITCH_MODE_MAX        SW_MODE_DISABLED      // xsx  SW_MODE_DISABLED, SW_MODE_HOMING, SW_MODE_LIMIT, SW_MODE_HOMING_LIMIT
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


// Machine settings
#define CHORDAL_TOLERANCE         0.01                  // chordal accuracy for arc drawing
#define SOFT_LIMIT_ENABLE         0                     // 0 = off, 1 = on
#define JERK_MAX                  10                    // yes, that's "20,000,000" mm/(min^3)
#define JUNCTION_DEVIATION        0.05                  // default value, in mm
#define JUNCTION_ACCELERATION     100000                // centripetal acceleration around corners


// Axis settings
#define VELOCITY_MAX             2000
#define FEEDRATE_MAX             4000

#define X_AXIS_MODE              AXIS_STANDARD        // xam  see canonical_machine.h cmAxisMode for valid values
#define X_VELOCITY_MAX           VELOCITY_MAX         // xvm  G0 max velocity in mm/min
#define X_FEEDRATE_MAX           FEEDRATE_MAX         // xfr  G1 max feed rate in mm/min
#define X_TRAVEL_MIN             0                    // xtn  minimum travel for soft limits
#define X_TRAVEL_MAX             150                  // xtm  travel between switches or crashes
#define X_JERK_MAX               JERK_MAX             // xjm
#define X_JERK_HOMING            (X_JERK_MAX * 2)     // xjh
#define X_JUNCTION_DEVIATION     JUNCTION_DEVIATION   // xjd
#define X_SEARCH_VELOCITY        500                  // xsv  move in negative direction
#define X_LATCH_VELOCITY         100                  // xlv  mm/min
#define X_LATCH_BACKOFF          5                    // xlb  mm
#define X_ZERO_BACKOFF           1                    // xzb  mm

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

#if 0
#define A_AXIS_MODE              AXIS_STANDARD
#define A_VELOCITY_MAX           VELOCITY_MAX
#define A_FEEDRATE_MAX           FEEDRATE_MAX
#define A_TRAVEL_MIN             0
#define A_TRAVEL_MAX             75
#define A_JERK_MAX               JERK_MAX
#define A_JERK_HOMING            (A_JERK_MAX * 2)
#define A_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define A_SEARCH_VELOCITY        400
#define A_LATCH_VELOCITY         100
#define A_LATCH_BACKOFF          5
#define A_ZERO_BACKOFF           1
#define A_RADIUS                 0

// A values are chosen to make the A motor react the same as X for testing
#else
#define A_AXIS_MODE              AXIS_RADIUS
#define A_VELOCITY_MAX           ((X_VELOCITY_MAX/M1_TRAVEL_PER_REV)*360) // set to the same speed as X axis
#define A_FEEDRATE_MAX           A_VELOCITY_MAX
#define A_TRAVEL_MIN             -1
#define A_TRAVEL_MAX             -1                                        // same number means infinite
#define A_JERK_MAX               (X_JERK_MAX * (360 / M1_TRAVEL_PER_REV))
#define A_JERK_HOMING            (A_JERK_MAX * 2)
#define A_JUNCTION_DEVIATION     JUNCTION_DEVIATION
#define A_RADIUS                 (M1_TRAVEL_PER_REV/(2*3.14159628))
#define A_SEARCH_VELOCITY         600
#define A_LATCH_VELOCITY          100
#define A_LATCH_BACKOFF           5
#define A_ZERO_BACKOFF            2
#endif

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
#define P1_PWM_FREQUENCY          100                 // in Hz
#define P1_CW_SPEED_LO            1000                // in RPM (arbitrary units)
#define P1_CW_SPEED_HI            2000
#define P1_CW_PHASE_LO            0.125               // phase [0..1]
#define P1_CW_PHASE_HI            0.2
#define P1_CCW_SPEED_LO           1000
#define P1_CCW_SPEED_HI           2000
#define P1_CCW_PHASE_LO           0.125
#define P1_CCW_PHASE_HI           0.2
#define P1_PWM_PHASE_OFF          0.1


// Gcode defaults
#define GCODE_DEFAULT_UNITS         MILLIMETERS      // MILLIMETERS or INCHES
#define GCODE_DEFAULT_PLANE         CANON_PLANE_XY   // CANON_PLANE_XY, CANON_PLANE_XZ, or CANON_PLANE_YZ
#define GCODE_DEFAULT_COORD_SYSTEM  G54              // G54, G55, G56, G57, G58 or G59
#define GCODE_DEFAULT_PATH_CONTROL  PATH_CONTINUOUS
#define GCODE_DEFAULT_DISTANCE_MODE ABSOLUTE_MODE


// Coordinate offsets
#define G54_X_OFFSET 0            // G54 is traditionally set to all zeros
#define G54_Y_OFFSET 0
#define G54_Z_OFFSET 0
#define G54_A_OFFSET 0
#define G54_B_OFFSET 0
#define G54_C_OFFSET 0

#define G55_X_OFFSET (X_TRAVEL_MAX / 2)    // set to middle of table
#define G55_Y_OFFSET (Y_TRAVEL_MAX / 2)
#define G55_Z_OFFSET 0
#define G55_A_OFFSET 0
#define G55_B_OFFSET 0
#define G55_C_OFFSET 0

#define G56_X_OFFSET 0
#define G56_Y_OFFSET 0
#define G56_Z_OFFSET 0
#define G56_A_OFFSET 0
#define G56_B_OFFSET 0
#define G56_C_OFFSET 0

#define G57_X_OFFSET 0
#define G57_Y_OFFSET 0
#define G57_Z_OFFSET 0
#define G57_A_OFFSET 0
#define G57_B_OFFSET 0
#define G57_C_OFFSET 0

#define G58_X_OFFSET 0
#define G58_Y_OFFSET 0
#define G58_Z_OFFSET 0
#define G58_A_OFFSET 0
#define G58_B_OFFSET 0
#define G58_C_OFFSET 0

#define G59_X_OFFSET 0
#define G59_Y_OFFSET 0
#define G59_Z_OFFSET 0
#define G59_A_OFFSET 0
#define G59_B_OFFSET 0
#define G59_C_OFFSET 0

#endif // CONFIG_H
