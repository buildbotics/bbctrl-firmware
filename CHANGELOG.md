Buildbotics CNC Controller Firmware Change Log
==============================================

## v0.3.6
 - Set max_usb_current=1 in /boot/config.txt from installer #103

## v0.3.5
 - Fixed dwell (G4)
 - Always show limit switch indicators regardless of motor enable
 - Fixed feed rate display
 - Added current GCode unit display
 - Fixed homed axis zeroing
 - Fixed probe pin input
 - Added reload button to video tab
 - Don't open error dialog on repeat messages
 - Handle large GCode files in browser
 - Added max lookahead limit to planner
 - Fixed GCode stopping/pausing where ramp down needs more than is in the queue
 - Added breakout box diagram to indicators
 - Initialize axes offsets to zero on startup
 - Fixed conflict between ``x`` state variable and ``x`` axis variable
 - Don't show ipv6 addresses on LCD.  They don't fit.

## v0.3.4
 - Added alternate units for motor parameters
 - Automatic config file upgrading
 - Fixed planner/jog sync
 - Fixed planner limits config
 - Accel units mm/min² -> m/min²
 - Search and latch velocity mm/min -> m/min
 - Fixed password update (broken in last version)
 - Start Web server eariler in case of Python coding errors
