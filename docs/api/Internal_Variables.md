# Buildbotics Controller Internal Variables

Internal variables may be read or written on the Buildbotics
controller via the API.  These variables are reported via the
Websocket interface at ``http://bbctrl.local/api/websocket``.

Some variables start with an index.  An index is a single
character appended to the front of the variable name and
indicates an offset into an array.  For example, the motor
variable ``0me`` is the motor enable value for motor 0.

These variable names are kept very short because they are used
for internal communication between the Buildbotics contoller's
internal RaspberryPi and AVR microcontroller.

A formal [JSON Schema](https://json-schema.org/) specification can be
found in the file [bbctrl-vars-schema.yaml](bbctrl-vars-schema.yaml).
Note, this is in YAML format but it can be easily converted to JSON if
needed.


## {index}an
**Full name**: motor_axis  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

Maps motor to axis.

## {index}me
**Full name**: motor_enabled  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Motor enabled. 0 for false, 1 for true.

## {index}pl
**Full name**: pulse_length  
**Index:** ``0123``  
**Type:** number  

length of step pulses.

## {index}dc
**Full name**: drive_current  
**Index:** ``0123``  
**Type:** number  

Max motor drive current.

## {index}ic
**Full name**: idle_current  
**Index:** ``0123``  
**Type:** number  

Motor idle current.

## {index}rv
**Full name**: reverse  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Reverse motor polarity. 0 for false, 1 for true.

## {index}mi
**Full name**: microstep  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Microsteps per full step.

## {index}vm
**Full name**: velocity_max  
**Index:** ``0123``  
**Type:** number  

Maximum vel in mm/min.

## {index}am
**Full name**: accel_max  
**Index:** ``0123``  
**Type:** number  

Maximum accel in mm/min^2.

## {index}jm
**Full name**: jerk_max  
**Index:** ``0123``  
**Type:** number  

Maximum jerk in mm/min^3.

## {index}sa
**Full name**: step_angle  
**Index:** ``0123``  
**Type:** number  

In degrees per full step.

## {index}tr
**Full name**: travel  
**Index:** ``0123``  
**Type:** number  

Travel in mm/rev.

## {index}tn
**Full name**: min_soft_limit  
**Index:** ``0123``  
**Type:** number  

Min soft limit.

## {index}tm
**Full name**: max_soft_limit  
**Index:** ``0123``  
**Type:** number  

Max soft limit.

## {index}h
**Full name**: homed  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Motor homed status. 0 for false, 1 for true.

## {index}ac
**Full name**: active_current  
**Index:** ``0123``  
**Type:** number  
**Read only**  

Motor current now.

## {index}df
**Full name**: driver_flags  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Motor driver flags.

## {index}en
**Full name**: encoder  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** -2147483648  
**Maximum:** 2147483647  
**Read only**  

Motor encoder.

## {index}ee
**Full name**: error  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** -2147483648  
**Maximum:** 2147483647  
**Read only**  

Motor position error.

## {index}tc
**Full name**: stall_current  
**Index:** ``0123``  
**Type:** number  

Stall detect current.

## {index}lm
**Full name**: stall_microstep  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Stall detect microsteps.

## {index}sl
**Full name**: driver_stalled  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  
**Read only**  

Motor stall status. 0 for false, 1 for true.

## {index}tv
**Full name**: stall_volts  
**Index:** ``0123``  
**Type:** number  

Motor BEMF threshold voltage.

## {index}sv
**Full name**: stall_velocity  
**Index:** ``0123``  
**Type:** number  

Stall velocity.

## {index}sp
**Full name**: stall_samp_time  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Stall sample time.

## fa
**Full name**: motor_fault  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  
**Read only**  

Motor fault status. 0 for false, 1 for true.

## {index}p
**Full name**: axis_position  
**Index:** ``xyzabc``  
**Type:** number  
**Read only**  

Axis position.

## {index}io
**Full name**: io_function  
**Index:** ``abcdefghijklmnopq``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

IO pin function map.

## {index}im
**Full name**: io_mode  
**Index:** ``abcdefghijklmnopq``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

IO pin mode.

## {index}is
**Full name**: io_state  
**Index:** ``abcdefghijklmnopq``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  
**Read only**  

IO pin state.

## sd
**Full name**: input_debounce  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Input debounce time in ms.

## sc
**Full name**: input_lockout  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Input lockout time in ms.

## {index}lw
**Full name**: min_input  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  
**Read only**  

Minimum switch input state.

## {index}xw
**Full name**: max_input  
**Index:** ``0123``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  
**Read only**  

Maximum switch input state.

## {index}w
**Full name**: input  
**Index:** ``0123ep``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  
**Read only**  

Digital input state.

## {index}oa
**Full name**: output_active  
**Index:** ``0123MFfedtb``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

Digital output active.

## {index}ai
**Full name**: analog_input  
**Index:** ``0123``  
**Type:** number  
**Read only**  

Analog input state.

## be
**Full name**: buffer_enable  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Buffer enable state. 0 for false, 1 for true.

## st
**Full name**: tool_type  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

See spindle.c.

## s
**Full name**: speed  
**Type:** number  
**Read only**  

Current spindle speed.

## sr
**Full name**: tool_reversed  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Reverse tool. 0 for false, 1 for true.

## sx
**Full name**: max_spin  
**Type:** number  

Maximum spindle speed.

## sm
**Full name**: min_spin  
**Type:** number  

Minimum spindle speed.

## ss
**Full name**: spindle_status  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  
**Read only**  

Spindle status code.

## pi
**Full name**: pwm_invert  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Inverted spindle PWM. 0 for false, 1 for true.

## nd
**Full name**: pwm_min_duty  
**Type:** number  

Minimum PWM duty cycle.

## md
**Full name**: pwm_max_duty  
**Type:** number  

Maximum PWM duty cycle.

## pd
**Full name**: pwm_duty  
**Type:** number  
**Read only**  

Current PWM duty cycle.

## sf
**Full name**: pwm_freq  
**Type:** number  

Spindle PWM frequency in Hz.

## hb
**Full name**: mb_debug  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Modbus debugging. 0 for false, 1 for true.

## hi
**Full name**: mb_id  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

Modbus ID.

## mb
**Full name**: mb_baud  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

Modbus BAUD rate.

## ma
**Full name**: mb_parity  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

Modbus parity.

## mx
**Full name**: mb_status  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  
**Read only**  

Modbus status.

## cr
**Full name**: mb_crc_errs  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  
**Read only**  

Modbus CRC error counter.

## vf
**Full name**: vfd_max_freq  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

VFD maximum frequency.

## mw
**Full name**: vfd_multi_write  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Use Modbus multi write mode. 0 for false, 1 for true.

## {index}vt
**Full name**: vfd_reg_type  
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

VFD register type.

## {index}va
**Full name**: vfd_reg_addr  
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

VFD register address.

## {index}vv
**Full name**: vfd_reg_val  
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

VFD register value.

## {index}vr
**Full name**: vfd_reg_fails  
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 255  

VFD register fail count.

## hz
**Full name**: hy_freq  
**Type:** number  
**Read only**  

Huanyang actual freq.

## hc
**Full name**: hy_current  
**Type:** number  
**Read only**  

Huanyang actual current.

## ht
**Full name**: hy_temp  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  
**Read only**  

Huanyang temperature.

## hx
**Full name**: hy_max_freq  
**Type:** number  
**Read only**  

Huanyang max freq.

## hm
**Full name**: hy_min_freq  
**Type:** number  
**Read only**  

Huanyang min freq.

## hq
**Full name**: hy_rated_rpm  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  
**Read only**  

Huanyang rated RPM.

## id
**Full name**: id  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  

Last executed command ID.

## fo
**Full name**: feed_override  
**Type:** number  

Feed rate override.

## so
**Full name**: speed_override  
**Type:** number  

Spindle speed override.

## jd
**Full name**: jog_id  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  
**Read only**  

Last completed jog command ID.

## v
**Full name**: velocity  
**Type:** number  
**Read only**  

Current velocity.

## ax
**Full name**: acceleration  
**Type:** number  
**Read only**  

Current acceleration.

## j
**Full name**: jerk  
**Type:** number  
**Read only**  

Current jerk.

## pv
**Full name**: peak_vel  
**Type:** number  

Peak velocity, set to clear.

## pa
**Full name**: peak_accel  
**Type:** number  

Peak accel, set to clear.

## dp
**Full name**: dynamic_power  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Dynamic power. 0 for false, 1 for true.

## if
**Full name**: inverse_feed  
**Type:** number  

Inverse feed rate.

## hid
**Full name**: hw_id  
**Type:** string  
**Read only**  

Hardware ID.

## es
**Full name**: estop  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 1  

Emergency stop. 0 for false, 1 for true.

## er
**Full name**: estop_reason  
**Type:** string  
**Read only**  

Emergency stop reason.

## xx
**Full name**: state  
**Type:** string  
**Read only**  

Machine state.

## xc
**Full name**: state_count  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 65535  
**Read only**  

Machine state change count.

## pr
**Full name**: hold_reason  
**Type:** string  
**Read only**  

Machine pause reason.

## un
**Full name**: underrun  
**Type:** integer  
**Minimum:** 0  
**Maximum:** 4294967295  
**Read only**  

Stepper buffer underrun count.

## dt
**Full name**: dwell_time  
**Type:** number  
**Read only**  

Dwell timer.

