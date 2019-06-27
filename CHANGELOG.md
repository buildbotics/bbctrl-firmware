Buildbotics CNC Controller Firmware Changelog
=============================================

## v0.4.10
 - Fix demo password check
 - Fix bug were fast clicks could cause jog commands to arrive out of order.
 - Fix bug bug where planner position may not sync after jog.

## v0.4.9
 - Enforce 6A per motor channel peak current limit.
 - Adjust config values above max or below min instead of resetting to default.

## v0.4.8
 - Fixed log rotating.
 - Use systemd serivce instead of init.d.
 - Fix planner terminate.
 - Changed AVR serial interrupt priorites.
 - Increased AVR serial and command buffers.
 - Boost HDMI signal.
 - Rewrote RPi serial driver.
 - Automatically scale max CPU speed to reduce RPi temp.
 - Disable USB camera if RPi temperature above 80°C, back on at 75°C.
 - Respect offsets in canned cycle moves.  #219
 - Fixed G53 warning.
 - Fixed delayed offset update after M2 or M30 end of program.
 - Handle multiple consecutive config resets correctly.
 - Fixed log CPU usage problem introduced in v0.4.6.
 - Show RPi temp on indicators page.
 - Show red thermometer if RPi temp exceeds 80°C.

## v0.4.7
 - Fix homing switch to motor channel mapping with non-standard axis order.
 - Added ``switch-debounce`` and ``switch-lockout`` config options.
 - Handle corrupt GCode simulation data correctly.
 - Fixes for exception logging.
 - Always limit motor max-velocity.  #209
 - Sync GCode and planner files to disk after write.
 - Added warning about reliability in a noisy environment on WiFi config page.
 - EStop on motor fault.
 - Fixed ETA line wrapping on Web interface.
 - Fixed zeroing with non-zero offset when unhomed. #211
 - Handle file paths uploaded from Windows correctly. #212
 - Don't retain estop state through reboot.
 - Log when RPi gets hot.
 - Support Modbus multi-write mode.
 - Added support for Nowforever VFDs.

## v0.4.6
 - Fixed a rare ``Negative s-curve time`` error.
 - Don't allow manual axis homing when soft limits are not set.
 - Right click to enable camera crosshair.
 - Demo mode.
 - Limit idle-current to 2A.
 - Removed dangerous ``power-mode`` in favor of simpler ``enabled`` option.
 - Fixed bug where motor driver could fail to disabled during estop.
 - Restored estop text.

## v0.4.5
 - Fix for random errors while running VFD.
 - Fix bug where planner would not continue after optional pause (M1).
 - Fix lockup on invalid no move probe G38.x. #183
 - Fix zeroing homed axis after jog.
 - Fix VFD communication at higher baud rates (> 9600). #184

## v0.4.4
 - Write version to log file.
 - Write time to log file periodically.
 - Show simulation progress with or with out 3D view.
 - Synchronize file list between browsers.
 - Increased max simulation time to 24hrs.
 - Added button to download current GCode file.
 - Blink play button to indicate pause.
 - Many layout tweaks/improvements.
 - Don't abort simulations when system time changes.
 - Only allow one camera stream at a time.

## v0.4.2
 - Suppress ``Auto-creating missing tool`` warning.
 - Prevent ``Stream is closed`` error.
 - Suppress ``WebGL not supported`` warning.
 - Fixed Web disconnect during simulation of large GCode.
 - Disable outputs on estop.
 - Improved switch debouncing for better homing.
 - Handle zero length dwell correctly.
 - Fixed problem with cached GCode file upload when file changed on disk.
 - Run simulation at low process priority.
 - Added ``Bug Report`` button to ``Admin`` -> ``General``.
 - Only render 3D view as needed to save CPU.
 - Prevent lockup due to browser causing out of memory condition.
 - Show error message when too large GCode upload fails.
 - Much faster 3D view loading.

## v0.4.1
 - Fix toolpath view axes bug.
 - Added LASER intensity view.
 - Fixed reverse path planner bug.
 - Video size and path view controls persistent over browser reload.
 - Fixed time and progress bugs.
 - Added PWM rapid auto off feature for LASER/Plasma.
 - Added dynamic PWM for LASER/Plasma.
 - Added motor faults table to indicators page.
 - Emit error and indicate FAULT on axis for motor driver faults.
 - Display axis motor FAULT on LCD.
 - Fixed bug with rapid repeated unpause.

## v0.4.0
 - Increased display precision of position and motor config.
 - Added support for 256 microstepping.
 - Smoother operation at 250k step rate by doubling clock as needed.
 - Indicators tab improvements.
 - Much improved camera support.
 - Camera hotpluging.
 - Move camera video to header.
 - Click to switch video size.
 - Automount/unmount USB drives.
 - Automatically install ``buildbotics.gc`` when no other GCode exists.
 - Preplan GCode and check for errors.
 - Display 3D view of program tool paths in browser.
 - Display accurate time remaining, ETA and progress during run.
 - Automatically collapse moves in planner which are too short in time.
 - Show IO status indicators on configuration pages.
 - Check that axis dimensions fit path plan dimensions.
 - Show machine working envelope in path plan viewer.
 - Don't reload browser view on reconnect unless controller has reloaded.
 - Increased max switch backoff search distance.
 - Major improvements for LASER raster GCodes.
 - Fixed major bug in command queuing.
 - Ignore Program Number O-Codes.
 - Improved planning of collinear line segments.
 - Allow PWM output up to 320kHz and no slower than 8Hz.

## v0.3.28
 - Show step rate on motor configuration page.
 - Limit motor max-velocity such that step rate cannot exceed 250k.
 - Fixed deceleration bug at full 250k step rate.

## v0.3.27
 - Fixed homing in imperial mode.

## v0.3.26
 - Removed VFD test.
 - Show VFD status on configuration page.
 - Show VFD commands fail counts.
 - Marked some VFD types as beta.

## v0.3.25
 - Error on home if max-soft-limit <= min-soft-limit + 1. #139
 - Decrease boot time networking delay.
 - Default to US keyboard layout. #145
 - Added configuration option to show metric or imperial units in browser. #74
 - Implemented fine jogging control in Web interface. #147

## v0.3.24
 - Added unhome button on axis position popup.
 - Ignore soft limits of max <= min.
 - Fixed problem with restarting program in imperial units mode.
 - Handle GCode with infinite or very long loops correctly.
 - Fixed Huanyang spindle restart after stop.

## v0.3.23
 - Fix for modbus read operation.
 - Finalized AC-Tech VFD support.
 - Preliminary FR-D700 VFD support.
 - Ignore leading zeros in modbus messages.
 - Handle older PWR firmwares.

## v0.3.22
 - Fix position loss after program pause.  #130
 - Correctly handle disabled axes.
 - Fixed config checkbox not displaying defaulted enabled correctly.
 - Added Custom Modbus VFD programming.

## v0.3.21
 - Implemented M70-M73 modal state save/restore.
 - Added support for modbus VFDs.
 - Start Huanyang spindle with out first pressing Start button on VFD.
 - Faster switching of large GCode files in Web.
 - Fixed reported gcode line off by one.
 - Disable MDI input while running.
 - Stabilized direction pin output during slow moves.

## v0.3.20
 - Eliminated drift caused by miscounting half microsteps.
 - Fixed disappearing GCode in Web.
 - More efficient GCode scrolling with very large files.
 - Fully functional soft-limited jogging.
 - Added client and access-point Wifi configuration.
 - Fixed broken hostname Web redirect after change.
 - Split admin page -> General & Network.
 - Improved calculation of junction velocity limits.

## v0.3.19
 - Fixed stopping problems. #127
 - Fixed ``Negative s-curve time`` error.
 - Improved jogging with soft limits.
 - Added site favicon.
 - Fixed problems with offsets and imperial units.
 - Fixed ``All zero s-curve times`` caused by extremely short, non-zero moves.
 - Fixed position drift.

## v0.3.18
 - Don't enable any tool by default.

## v0.3.17
 - Fixed pausing fail near end of run bug.
 - Show "Upgrading firmware" when upgrading.
 - Log excessive pwr communication failures as errors.
 - Ensure we can still get out of non-idle cycles when there are errors.
 - Less frequent pwr variable updates.
 - Stop cancels seek and subsequent estop.
 - Fixed bug in AVR/Planner command synchronization.
 - Consistently display HOMING state during homing operation.
 - Homing zeros axis global offset.
 - Added zero all button. #126
 - Separate "Auto" and "MDI" play/pause & stop buttons. #126
 - Moved home all button. #126
 - Display "Video camera not found." instead of broken image icon.
 - Show offset positions not absolute on LCD.
 - Don't change gcode lines while homing.
 - Don't change button states while homing.
 - Adding warning about power cyclying during an upgrade.
 - Reset planner on AVR errors.
 - Fixed pausing with short moves.
 - Corrected s-curve accel increasing jogging velocities.

## v0.3.16
 - Fixed switch debounce bug.

## v0.3.15
 - Suppress warning missing config.json warning after config reset.
 - Fixed EStop reboot loop.
 - Removed AVR unexpected reboot error.

## v0.3.14
 - Fixed: Config fails silently after web disconnect #112
 - Always reload the page after a disconnect.
 - Honor soft limits #111 (but not when jogging)
 - Limit switch going active while moving causes estop. #54
 - Added more links to help page.
 - Fixed axis display on LCD. #122
 - Added GCode cheat sheet.
 - Fixed LCD boot splash screen. #121
 - Implemented tool change procedures and pause message box. #81
 - Implemented program start and end procedures.

## v0.3.13
 - Disable spindle and loads on stop.
 - Fixed several state transition (stop, pause, estop, etc.) problems.

## v0.3.12
 - Updated DB25 M2 breakout diagram.
 - Enabled AVR watchdog.
 - Fixed problem with selecting newly uploaded file.
 - More thorough shutdown of stepper driver in estop.
 - Fixed spindle type specific options.
 - No more ``Unexpected AVR firmware reboot`` errors on estop clear.
 - Downgraded ``Machine alarmed - Command not processed`` errors to warnings.
 - Suppress unnecessary axis homing warnings.
 - More details for axis homing errors.
 - Support GCode messages e.g. (MSG, Hello World!)
 - Support programmed pauses.  i.e. M0

## v0.3.11
 - Suppressed ``firmware rebooted`` warning.
 - Error on unexpected AVR reboot.
 - Fixed pin fault output.
 - No longer using interrupts for switch inputs.  Debouncing on clock tick.

## v0.3.10
 - Fixed "Flood" display, changed to "Load 1" and "Load 2".  #108
 - Highlight loads when on.
 - Fixed axis zeroing.
 - Fixed bug in home position set after successful home.  #109
 - Fixed ugly Web error dumps.
 - Allow access to log file from Web.
 - Rotate log so it does not grow too big.
 - Keep same GCode file through browser reload.  #20

## v0.3.9
 - Fixed bug in move exec that was causing bumping between moves.
 - Fixed planner bug which could create negative s-curve times.
 - Hide step and optional pause buttons until they are implemented.
 - Fixed pausing problems.
 - Limit number of console messages.
 - Scrollbar on console view.
 - Log debug messages to console in developer mode.
 - Fixed AVR log message source.
 - Fixed step correction.
 - JOGGING, HOMMING and MDI states.
 - Fixed position problem with rapid MDI entry.

## v0.3.8
 - Fixed pwr flags display
 - Added pwr fault flags to indicators

## v0.3.7
 - Allow blocking error dialog for a period of time
 - Show actual error message on planner errors
 - Reset planner on serious error
 - Fixed console clear
 - Added helpful info to Video tab
 - Changed "Console" tab to "Messages"
 - Removed spin up/down velocity options, they don't do anything
 - Allow RS485 to work when wires are swapped
 - Allow setting VFD ID
 - Only show relevant spindle config items
 - More robust video camera reset
 - Added help page
 - Allow upgrade with out Internet
 - Limit power fault reporting
 - Added load over temp, load limiting and motor overload power faults

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
 - Start Web server earlier in case of Python coding errors


Changelog not maintained in previous versions.  See git commit log.
