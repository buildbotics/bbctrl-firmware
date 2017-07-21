# Notes on the lifecycle of a movement

## Parsing
A move first starts off as a line of GCode which is parsed by
``gc_gcode_parser()`` which in turn calls ``_normalize_gcode_block()``
and ``_parse_gcode_block()``.  ``_parse_gcode_block()`` sets flags in
``mach.gf`` indicating which values have changed and the changed values in
``mach.gn``.  ``_parse_gcode_block()`` then calls ``_execute_gcode_block()``
which calls ``mach_*()`` functions which modify the state of the GCode machine.

## Queuing
Some functions such as ``mach_straight_traverse()``, ``mach_straight_feed()``
and ``mach_arc_feed()`` result in line moves being entered into the planner
queue.  Others enter dwells or commands or into the queue.  Planner buffer
entries encode everything needed to execute a move with out referring back to
the machine model.

Line moves are entered into the queue by calls to ``mp_aline()``.  Arcs are
converted to short straight line moves and are feed in as buffer room becomes
available, until complete, via calls to ``mach_arc_callback()`` which results in
multiple calls to ``mp_aline()``.

``mp_aline()`` plans straight line movements by first calling
``mach_calc_move_time()`` which uses the current GCode state to estimate the
total time required to complete the move at the current feed rate and feed rate
mode.  If the move time is long enough, a buffer is allocated.  Its jerk, max
cruise velocity, max entry velocity, max delta velocity, max exit velocity and
breaking velocity are all computed.  The move velocities are planned by a
call to ``mp_plan_block_list()``.  Initially ``bf_func`` is set to
``mp_exec_aline()`` and the buffer is committed to the queue by calling
``mp_commit_write_buffer()``.

## Planning
### Backward planning pass
``mp_plan_block_list()`` plans movements by first moving backwards through the
planning buffer until either the last entry is reached or a buffer marked not
``replannable`` is encountered.  The ``breaking_velocity`` is propagated back
during the backwards pass.  Next, begins the forward planning pass.

### Forward planning pass
During the forward pass the entry velocity, cruise velocity and exit velocity
are computed and ``mp_calculate_trapezoid()`` is called to (re)compute the
velocity trapezoids of each buffer being considered.  If a buffer's plan is
deemed optimal then it is marked not ``replannable`` to avoid replanning later.

### Trapezoid planning
The call to ``mp_calculate_trapezoid()`` computes head, body and tail lengths
for a single planner buffer.  Entry, cruse and exit velocities may be modified
to make the trapezoid fit with in the move length.  Planning may result in a
degraded trapezoid.  I.e. one with out all three sides.

## Execution
The stepper motor driver interrupt routine calls ``mp_exec_move()`` to prepare
the next move for execution.  ``mp_exec_move()`` access the next buffer in the
planner queue and calls the function pointed to by ``bf_func`` which is
initially set to ``mp_exec_aline()`` during planning.  Each call to
``mp_exec_move()`` must prepare one and only one move fragment for the stepper
driver.  The planner buffer is executed repeatedly as long as ``bf_func``
returns ``STAT_EAGAIN``.

### Move initialization
On the first call to ``mp_exec_aline()`` a call is made to
``_exec_aline_init()``.  This function may stop processing the move if a
feedhold is in effect.  It may also skip a move if it has zero length.
Otherwise, it initializes the move runtime state (``mr``) copying over the
variables set in the planner buffer.  In addition, it computes waypoints at
the ends of each trapezoid section.  Waypoints are used later to correct
position for rounding errors.

### Move execution
After move initialization ``mp_exec_aline()`` calls ``_exec_aline_head()``,
``_exec_aline_body()`` and ``exec_aline_tail()`` on successive callbacks.
These functions are called repeatedly until each section finishes.  If any
sections have zero length they are skipped and execution is passed immediately
to the next section.  During each section, forward difference is used to map
the trapezoid computed during the planning stage to a fifth-degree Bezier
polynomial S-curve.  The curve is used to find the appropriate velocity at the
next target position.

``_exec_aline_segment()`` is called for each non-zero section to convert the
computed target position to target steps by calling ``mp_kinematics()``.  The
move fragment is then passed to the stepper driver by a call to
``st_prep_line()``.  When a segment is complete ``_exec_aline_segment()``
returns ``STAT_OK`` indicating the next segment should be loaded.  When all
non-zero segments have been executed, the move is complete.

## Pulse generation
Calls to ``st_prep_line()`` prepare short (~5ms) move fragments for pulse
generation by the stepper driver.  Move time in clock ticks is computed from
travel in steps and move duration.  Then ``motor_prep_move()`` is called for
each motor. ``motor_prep_move()`` may perform step correction and enable the
motor.  It then computes the optimal step clock divisor, clock ticks and sets
the move direction, taking the motor's configuration in to account.

The stepper timer interrupt, after ending the previous move, calls
``motor_load_move()`` on each motor.  This sets up and starts the motor clocks,
sets the motor direction lines and accumulates and resets the step encoders.
After (re)starting the motor clocks the interrupt signals a lower level
interrupt to call ``mp_exec_move()`` and load the next move fragment.
