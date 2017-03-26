# Parsing, Queuing & Planning
 * command_callback()
   * gc_gcode_parser(const char *block)
     * _normalize_gcode_block(...)
     * _parse_gcode_block(const char *block)
       * _execute_gcode_block()
         * mach_*()
           * mach_straight_traverse() || mach_straight_feed() || mach_arc_feed()
             * mp_aline(const float target[], float feed,. . .)
               * _calc_jerk*()
               * _calc_move_time()
               * _calc_max_velocities()
                 * _get_junction_vmax()
               * mp_plan()
                 * mp_calculate_trapezoid()
                   * mp_get_target_length()
                   * mp_get_target_velocity()
               * mp_queue_push(buffer_cb_t cb, uint32_t time)

# Execution
 * Stepper low-level ISR
   * mp_exec_move()
     * mp_queue_get_head()
     * mp_buffer->cb()
       * _exec_dwell()
       * mp_exec_aline()
         * _exec_aline_init()
         * _exec_aline_head() || _exec_aline_body() || _exec_aline_tail()
           * _exec_aline_section()
             * mp_init_forward_dif() || mp_next_forward_dif()
             * _exec_aline_segment()
               * mp_runtime_move_to_target()
                 * mp_kinematics() - Converts target in mm to steps
                 * st_prep_line()
                   * motor_prep_move()

# Step Output
 * STEP_TIMER_ISR
   * motor_end_move()
   * _request_exec_move()
     * Triggers Stepper low-level ISR
   * motor_load_move()


# Data flow
## GCode block
char *line

## Planner buffer
 * mp_buffer_t pool[]
   * float target[]                             - Target position in mm
   * float unit[]                               - Direction vector
   * float length, {head,body,tail}_length      - Lengths in mm
   * float {entry,cruise,exit,braking}_velocity - Target velocities in mm/min
   * float {entry,cruise,exit,delta}_vmax       - Max velocities in mm/min
   * float jerk                                 - Max jerk in km/min^3

``mach_*()`` functions compute ``target`` and call ``mp_aline()``.
``mp_aline()`` saves ``target`` computes ``unit`` and ``jerk``.  Then it
estimates the move time in order to calculate max velocities.

``mp_plan()`` and friends compute target velocities and trapezoid segment
lengths in two passes using max velocities in the current and neighboring
planner buffers.

``mp_exec_aline()`` executes the trapezoidal move in the next planner buffer.

## Move prep
 * stepper_t st
   * uint16_t seg_period   - The step timer period
 * motor_t motors[]
   * uint8_t timer_clock   - Clock divisor or zero for off
   * uint16_t timer_period - Clock period
   * direction_t direction - Step polarity
