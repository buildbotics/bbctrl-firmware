# Dual-Motor Homing (Gantry Squaring)

Machines with two motors driving one axis — typically a gantry router with a
motor on each side of the Y axis — can rack out of square when the machine is
powered off or loses steps.  Homing both motors to the same single switch
cannot correct this: the motors are driven in lockstep, so whatever skew the
gantry has when homing starts, it keeps.

Dual-motor homing gives each motor its own limit switch.  During homing, the
first motor to reach its switch holds position while the other keeps moving
until it reaches its own switch.  The mechanical skew between the two sides is
measured and locked in as a per-motor offset, and the gantry comes out square.
After homing the motors move in lockstep again, offset intact, until the axis
is unhomed.

## Requirements

- Two motors mapped to the same axis (e.g. motors 1 and 3 both set to
  ``Y``).  The second motor is the *slave* and inherits most settings from
  the first.
- A separate limit switch wired to each motor's switch input, both on the
  homing side of the axis (both min or both max).
- ``switch-min`` or ``switch-max`` homing on the first motor.  Stall homing
  cannot square an axis and is not supported.

## Configuration

All squaring settings live on the **first** motor of the pair (the lower
motor number).  They are hidden on the slave motor's settings page.

1. On the second motor's page, set its axis to match the first motor's.
2. Under **IO**, map an input pin to each motor's homing-side switch
   function.  For motors 1 and 3 homing to min, that is
   ``input-motor-1-min`` and ``input-motor-3-min``.
3. On the first motor's **homing** settings, pick ``switch-min`` or
   ``switch-max`` and enable **square-axis**.
4. Set **max-square-error** (default 10 mm) to a little more than the worst
   racking you expect.  See below.

Homing the axis (or homing all) now homes both motors.  The log reads
``Homing and squaring Y axis`` when squaring is active.

## max-square-error

While squaring, one motor is stopped at its switch and the other is still
moving; a wiring fault on the second switch would otherwise drive the gantry
further out of square until the axis runs out of travel.  If the distance
between the two motors exceeds ``max-square-error`` before the second switch
triggers, homing stops with ``Squaring limit exceeded``.

- Too small and normal racking trips it: set it above the skew your machine
  actually accumulates.
- Zero disables the check.  Not recommended.

## First run

Before trusting it:

1. Set ``max-square-error`` low, 2–3 mm, and keep a hand near EStop.
2. Home the axis.  Both motors should stop at their own switches.
3. Confirm the measured offset is sane: the AVR variable ``qo`` on each
   motor reports the squaring offset in mm.
4. Trip one switch by hand mid-seek (or disconnect it) and confirm homing
   aborts with ``Squaring limit exceeded`` instead of racking the gantry.

## How it works

The controller sends the AVR a seek command carrying both switch IDs.  When a
switch triggers, the firmware puts that motor in a *homing hold*: the motor
stops stepping (but stays powered) and the axis motion it ignores accumulates
as its squaring offset.  When the second switch triggers, both motors are
released and decelerate together.  The offset is applied to every subsequent
move and survives ``G28.3``; unhoming the axis (``G28.2`` or a homing retry)
clears it on both motors.

The latch back-off and slow re-probe phases of switch homing run after the
squaring seek, so the final accuracy of the home position comes from the
normal latch procedure; squaring accuracy comes from the relative placement
of the two switches.  If the gantry homes square but not *perfectly* square,
adjust one switch's physical position.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| ``square-axis needs a second enabled motor...`` in log | Second motor not mapped to the axis, or disabled |
| ``square-axis requires switch-min or switch-max homing`` | First motor uses manual or stall homing |
| ``Squaring limit exceeded`` | Second switch never triggered (wiring, dead switch), or ``max-square-error`` set below real racking |
| Axis homes but stays racked | Both switch functions mapped to the same physical pin, or slave switch on the wrong side |
| EStop during homing | One of the pair's switches not mapped under IO, so its trigger is treated as an unexpected limit |
