# Buildbotics Controller Configuration Variables

Configuration variables are set on the Buildbotics controller
via a JSON configuration file.  The controller may be configured
by uploading a configuration file via the administration panel of
web interface or via the API.  Individual configuration options may
also set via the web interface or the API.

Configuration variables are organized into categories and subcategories.
The variable type is noted and where appropriate, units, and minium and
maximum values.

Some variables start with an index.  An index is a single character
appended to the front of the variable name and indicates an offset into
an array.  For example, the motor variable ``0microsteps`` is the microstep
value for motor 0.

A formal [JSON Schema](https://json-schema.org/) specification can be
found in the file [bbctrl-config-schema.yaml](bbctrl-config-schema.yaml).
Note, this is in YAML format but it can be easily converted to JSON if needed.


## settings
### units
**Type:** string  
**Enum:** ``METRIC``, ``IMPERIAL``  
**Default:** ``METRIC``  

### max-deviation
**Type:** number  
**Units:** mm or in  
**Minimum:** ``0.001``  
**Maximum:** ``100``  
**Default:** ``0.1``  

Default allowed deviation from programmed path.  Also see G64 & G61.

### junction-accel
**Type:** number  
**Units:** mm/min² or in/min²  
**Minimum:** ``10000``  
**Maximum:** ``100000000``  
**Default:** ``200000``  

Higher values will increase cornering speed but may cause stalls.

## motors
Motor configuration variables are index by the motor number.

### general
#### {index}axis
**Index:** ``0123``  
**Type:** string  
**Enum:** ``X``, ``Y``, ``Z``, ``A``, ``B``, ``C``  
**Default:** ``X``  

### driver
#### {index}type-of-driver
**Index:** ``0123``  
**Type:** string  
**Enum:** ``internal``, ``generic external``  
**Default:** ``internal``  

#### {index}step-length
**Index:** ``0123``  
**Type:** number  
**Units:** seconds  
**Minimum:** ``0``  
**Default:** ``2e-06``  

### power
#### {index}enabled
**Index:** ``0123``  
**Type:** boolean  
**Default:** ``True``  

#### {index}drive-current
**Index:** ``0123``  
**Type:** number  
**Units:** amps  
**Minimum:** ``0``  
**Maximum:** ``6``  
**Default:** ``1.5``  

#### {index}idle-current
**Index:** ``0123``  
**Type:** number  
**Units:** amps  
**Minimum:** ``0``  
**Maximum:** ``2``  

### motion
#### {index}reverse
**Index:** ``0123``  
**Type:** boolean  

#### {index}microsteps
**Index:** ``0123``  
**Type:** integer  
**Units:** per full step  
**Enum:** ``1``, ``2``, ``4``, ``8``, ``16``, ``32``, ``64``, ``128``, ``256``  
**Default:** ``32``  

#### {index}max-velocity
**Index:** ``0123``  
**Type:** number  
**Units:** m/min or IPM  
**Minimum:** ``0``  
**Default:** ``5``  

#### {index}max-accel
**Index:** ``0123``  
**Type:** number  
**Units:** km/min² or g-force  
**Minimum:** ``0``  
**Default:** ``10``  

#### {index}max-jerk
**Index:** ``0123``  
**Type:** number  
**Units:** km/min³ or g/min  
**Minimum:** ``0``  
**Default:** ``50``  

#### {index}step-angle
**Index:** ``0123``  
**Type:** number  
**Units:** degrees  
**Minimum:** ``0``  
**Maximum:** ``360``  
**Default:** ``1.8``  

#### {index}travel-per-rev
**Index:** ``0123``  
**Type:** number  
**Units:** mm or in  
**Default:** ``5``  

### limits
#### {index}min-soft-limit
**Index:** ``0123``  
**Type:** number  
**Units:** mm or in  

#### {index}max-soft-limit
**Index:** ``0123``  
**Type:** number  
**Units:** mm or in  

### homing
#### {index}homing-mode-external
**Index:** ``0123``  
**Type:** string  
**Enum:** ``manual``, ``switch-min``, ``switch-max``  
**Default:** ``manual``  

#### {index}homing-mode
**Index:** ``0123``  
**Type:** string  
**Enum:** ``manual``, ``switch-min``, ``switch-max``, ``stall-min``, ``stall-max``  
**Default:** ``manual``  

#### {index}stall-microstep
**Index:** ``0123``  
**Type:** integer  
**Units:** per full step  
**Enum:** ``2``, ``4``, ``8``, ``16``, ``32``, ``64``, ``128``, ``256``  
**Homing modes:** ``stall-min``, ``stall-max``  
**Default:** ``8``  

#### {index}search-velocity
**Index:** ``0123``  
**Type:** number  
**Units:** m/min or IPM  
**Homing modes:** ``switch-min``, ``switch-max``, ``stall-min``, ``stall-max``  
**Minimum:** ``0``  
**Default:** ``0.5``  

The homing latch search speed in m/min.

#### {index}latch-velocity
**Index:** ``0123``  
**Type:** number  
**Units:** m/min or IPM  
**Homing modes:** ``switch-min``, ``switch-max``  
**Minimum:** ``0``  
**Default:** ``0.1``  

The when backing off the latch or reprobing speed in m/min.

#### {index}latch-backoff
**Index:** ``0123``  
**Type:** number  
**Units:** mm or in  
**Homing modes:** ``switch-min``, ``switch-max``  
**Minimum:** ``0``  
**Default:** ``100``  

The distance in mm to move away from a latch before the more accurate and slower reprobe.

#### {index}stall-volts
**Index:** ``0123``  
**Type:** number  
**Units:** v  
**Homing modes:** ``stall-min``, ``stall-max``  
**Minimum:** ``0``  
**Default:** ``6``  

#### {index}stall-sample-time
**Index:** ``0123``  
**Type:** integer  
**Units:** µsec  
**Enum:** ``50``, ``100``, ``200``, ``300``, ``400``, ``600``, ``800``, ``1000``  
**Homing modes:** ``stall-min``, ``stall-max``  
**Default:** ``50``  

#### {index}stall-current
**Index:** ``0123``  
**Type:** number  
**Units:** amps  
**Homing modes:** ``stall-min``, ``stall-max``  
**Minimum:** ``0``  
**Default:** ``1.5``  

#### {index}zero-backoff
**Index:** ``0123``  
**Type:** number  
**Units:** mm or in  
**Homing modes:** ``switch-min``, ``switch-max``, ``stall-min``, ``stall-max``  
**Minimum:** ``0``  
**Default:** ``5``  

The distance in mm to move away from a latch after the final probe.

## tool
### tool-type
**Type:** string  
**Enum:** ``Disabled``, ``PWM Spindle``, ``Huanyang VFD``, ``Custom Modbus VFD``, ``AC-Tech VFD``, ``Nowforever VFD``, ``Delta VFD015M21A (Beta)``, ``YL600, YL620, YL620-A VFD (Beta)``, ``FR-D700 (Beta)``, ``Sunfar E300 (Beta)``, ``OMRON MX2``, ``V70``, ``H100``, ``WJ200``, ``DMM DYN4 (Beta)``, ``Galt G200/G500``, ``Teco Westinghouse E510``, ``EM60``, ``Fuling DZB200/300``  
**Default:** ``Disabled``  

### tool-reversed
**Type:** boolean  

### max-spin
**Type:** number  
**Units:** RPM  
**Minimum:** ``0``  
**Default:** ``10000``  

### min-spin
**Type:** number  
**Units:** RPM  
**Minimum:** ``0``  

## modbus-spindle
### bus-id
**Type:** integer  
**Default:** ``1``  

### baud
**Type:** integer  
**Enum:** ``9600``, ``19200``, ``38400``, ``57600``, ``115200``  
**Default:** ``9600``  

### parity
**Type:** string  
**Enum:** ``None``, ``Even``, ``Odd``  
**Default:** ``None``  

### multi-write
**Type:** boolean  

Use Modbus multi register write.  Function 16 vs. 6.

### regs
ModBus registers are index by alphanumeric characters.

#### {index}reg-type
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** string  
**Enum:** ``disabled``, ``connect-write``, ``max-freq-read``, ``max-freq-fixed``, ``freq-set``, ``freq-signed-set``, ``freq-scaled-set``, ``stop-write``, ``forward-write``, ``reverse-write``, ``freq-read``, ``freq-signed-read``, ``freq-actech-read``, ``status-read``, ``disconnect-write``  
**Default:** ``disabled``  

#### {index}reg-addr
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** integer  
**Minimum:** ``0``  
**Maximum:** ``65535``  

#### {index}reg-value
**Index:** ``0123456789abcdefghijklmnopqrstuv``  
**Type:** integer  
**Minimum:** ``0``  
**Maximum:** ``65535``  

## pwm-spindle
### pwm-inverted
**Type:** boolean  

Invert the PWM signal output.

### pwm-min-duty
**Type:** number  
**Units:** %  
**Minimum:** ``0``  
**Maximum:** ``100``  
**Default:** ``1``  

### pwm-max-duty
**Type:** number  
**Units:** %  
**Minimum:** ``0``  
**Maximum:** ``100``  
**Default:** ``99.99``  

### pwm-freq
**Type:** integer  
**Units:** Hz  
**Minimum:** ``8``  
**Maximum:** ``320000``  
**Default:** ``1000``  

### rapid-auto-off
**Type:** boolean  

Turn tool off during rapid moves.  Useful for LASERs.

### dynamic-power
**Type:** boolean  

Adjust tool power based on velocity and feed rate.  Useful for LASERs.

## io-map
IO Map entries are indexed by letters.

### {index}function
**Index:** ``abcdefghijklmnopq``  
**Type:** string  
**Enum:** ``disabled``, ``input-motor-0-max``, ``input-motor-1-max``, ``input-motor-2-max``, ``input-motor-3-max``, ``input-motor-0-min``, ``input-motor-1-min``, ``input-motor-2-min``, ``input-motor-3-min``, ``input-0``, ``input-1``, ``input-2``, ``input-3``, ``input-estop``, ``input-probe``, ``output-0``, ``output-1``, ``output-2``, ``output-3``, ``output-mist``, ``output-flood``, ``output-fault``, ``output-tool-enable``, ``output-tool-direction``, ``analog-0``, ``analog-1``, ``analog-2``, ``analog-3``  
**Default:** ``disabled``  

### {index}mode
**Index:** ``abcdefghijklmnopq``  
**Type:** string  
**Enum:** ``lo-hi``, ``hi-lo``, ``tri-lo``, ``tri-hi``, ``lo-tri``, ``hi-tri``, ``normally-closed``, ``normally-open``  
**Default:** ``hi-lo``  

## input
### input-debounce
**Type:** integer  
**Units:** ms  
**Minimum:** ``1``  
**Maximum:** ``5000``  
**Default:** ``5``  

Minimum time in ms before a switch change is acknowledged.

### input-lockout
**Type:** integer  
**Units:** ms  
**Minimum:** ``0``  
**Maximum:** ``60000``  
**Default:** ``250``  

Time in ms to ignore switch changes after an acknowledge change.

## gcode
### program-start
**Type:** string  
**Default:**
```
(Runs at program start)
G90 (Absolute distance mode)
G17 (Select XY plane)

```

### tool-change
**Type:** string  
**Default:**
```
(Runs on M6, tool change)
M0 M6 (MSG, Change tool)
```

### program-end
**Type:** string  
**Default:**
```
(Runs on M2 or M30, program end)
M2
```

## admin
### virtual-keyboard-enabled
**Type:** boolean  
**Default:** ``True``  

### auto-check-upgrade
**Type:** boolean  
**Default:** ``True``  

## macros
**Type:** array  

